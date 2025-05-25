#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

// ------ OLED ------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ------ MPU6050 ------
Adafruit_MPU6050 mpu;

// ------ Sayfa ------
volatile int page = 0;

// ------ Wi-Fi ------
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// ------ Hava Durumu API ------
const char* apiKey = "YOUR_API_KEY";
const char* city = "Istanbul";

// ------ Bilgisayar Mesajı ------
const char* mesajServer = "YOUR_SERVER_IP";
const int mesajPort = YOUR_PORT;
const char* mesajPath = "/your_path.html";
String bildirim = "";

// ------ Saat ------
const long gmtOffset_sec = 3 * 3600;
const int daylightOffset_sec = 0;

// ------ Zaman ------
unsigned long previousMillis = 0;
const long interval = 1000;
unsigned long lastTiltChange = 0;

// ------ Veriler ------
String weatherDescription = "";
float temperature = 0.0;
String depremYer = "";
float depremBuyukluk = 0.0;

// ------ Eye Parametreleri ------
int eye_width = 20;
int eye_height = 16;

// ------ Sleep Mode Değişkenleri ------
bool isSleeping = false;
unsigned long tiltUpStart = 0;
unsigned long tiltRightStart = 0;
bool tiltUpDetected = false;
bool tiltRightDetected = false;

float usdRate = 0.0;
float eurRate = 0.0;
volatile unsigned long adimSayisi = 0;
float previousAcceleration = 0.0;
unsigned long lastStepTime = 0;
int kaydirmaIndex = 0;
unsigned long kaydirmaMillis = 0;
String kaydirmaMetni = "";

void wakeup_eyes();
void getWeather();
void getMesaj();
void getDoviz();
void checkTilt();
void checkSteps();
void enterSleepMode();
void exitSleepMode();
void getDeprem();

void setup() {
  Wire.begin(D2, D1);

  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED bulunamadi");
    while (1);
  }

  if (!mpu.begin()) {
    Serial.println("MPU6050 bulunamadi!");
    while (1);
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println("Looking for Wi-Fi...");
  display.display();

  WiFi.begin(ssid, password);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Wi-Fi is connected!");
  display.clearDisplay();
  display.setCursor(0, 10);
  display.println("Wi-Fi is connected!");
  display.display();
  delay(1000);

  // ---- UYANMA ANİMASYONU ----
  display.clearDisplay();
  display.setCursor(0, 10);
  display.println("Starting... hopefully");
  display.display();
  delay(1000);

  wakeup_eyes();

  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");

  getWeather();
  getMesaj();
  getDoviz();
  getDeprem();
  showPage();
}

void yaziyiKaydirVeYaz(String metin, int y) {
  if (metin.length() <= 20) {
    display.setCursor(0, y);
    display.println(metin);
  } else {
    String kaydirilan = metin.substring(kaydirmaIndex);

    if (kaydirilan.length() < 20) {
      kaydirilan += " " + metin.substring(0, 20 - kaydirilan.length());
    }

    display.setCursor(0, y);
    display.println(kaydirilan);
  }
}

void loop() {
  static unsigned long lastTiltCheck = 0;
  if (millis() - lastTiltCheck >= 500) {
    checkTilt();
    checkSteps();
    lastTiltCheck = millis();
  }

  // --- GENEL YAZI KAYDIRMA ---
  if (millis() - kaydirmaMillis > 300) {
    kaydirmaMillis = millis();

    if (kaydirmaMetni.length() > 20) {
      kaydirmaIndex++;
      if (kaydirmaIndex >= kaydirmaMetni.length()) {
        kaydirmaIndex = 0;
      }
    } else {
      kaydirmaIndex = 0;
    }
  }

  if (!isSleeping) {

    // Sayfa 0: Saat ve Tarih
    if (page == 0) {
      unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        showPage();
      }
    }

    // Sayfa 1: Hava Durumu
    else if (page == 1) {
      static unsigned long weatherUpdateMillis = 0;
      if (millis() - weatherUpdateMillis > 120000) { // 2 dakikada bir güncelle
        weatherUpdateMillis = millis();
        getWeather();
      }
      showPage();
    }

    // Diğer sayfalar
    else {
      showPage();
      delay(300);
    }

    // Bilgisayar mesajını güncelle (5 saniyede bir)
    static unsigned long mesajUpdateMillis = 0;
    if (millis() - mesajUpdateMillis > 5000) {
      mesajUpdateMillis = millis();
      getMesaj();
    }

    // Döviz kuru güncelle (5 dakikada bir)
    static unsigned long dovizUpdateMillis = 0;
    if (millis() - dovizUpdateMillis > 300000) {
      dovizUpdateMillis = millis();
      getDoviz();
    }

    // Deprem bilgisi güncelle (5 dakikada bir)
    static unsigned long depremUpdateMillis = 0;
    if (millis() - depremUpdateMillis > 300000) {
      depremUpdateMillis = millis();
      getDeprem();
    }
  }
}


void getDoviz() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    String serverPath = "https://v6.exchangerate-api.com/v6/YOUR_API_KEY/latest/TRY";

    http.begin(client, serverPath);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String payload = http.getString();

      DynamicJsonDocument doc(4096);
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        usdRate = doc["conversion_rates"]["USD"].as<float>();
        eurRate = doc["conversion_rates"]["EUR"].as<float>();
      }

    }

    http.end();
  }
}

// ------ SAYFALAR ------
void showPage() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (page == 0) {
    display.setCursor(0, 0);
    display.println("Saat & Tarih:");
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      display.setCursor(0, 10);
      display.println("Zaman alinamadi");
    } else {
      char buffer[20];
      strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
      display.setCursor(0, 10);
      display.print("Saat: ");
      display.println(buffer);
      strftime(buffer, sizeof(buffer), "%d/%m/%Y", &timeinfo);
      display.setCursor(0, 20);
      display.print("Tarih: ");
      display.println(buffer);
    }
    kaydirmaMetni = "";  // Kaydırılacak yazı yok

  } else if (page == 1) {
    display.setCursor(0, 0);
    display.println("Hava Durumu:");
    display.setCursor(0, 10);
    display.print("Sicaklik: ");
    display.print(temperature);
    display.println(" C");
    display.setCursor(0, 20);
    display.print("Durum: ");
    display.println(weatherDescription);
    kaydirmaMetni = "";  // Hava durumu sabit yazı

  } else if (page == 2) {
    display.setCursor(0, 0);
    display.println("Doviz Kuru:");

    display.setCursor(0, 10);
    display.print("1 USD = ");
    if (usdRate > 0.0) {
      display.print(1 / usdRate, 2);
      display.println(" TL");
    } else {
      display.println("Yok");
    }

    display.setCursor(0, 20);
    display.print("1 EUR = ");
    if (eurRate > 0.0) {
      display.print(1 / eurRate, 2);
      display.println(" TL");
    } else {
      display.println("Yok");
    }
    kaydirmaMetni = "";  // Döviz sabit yazı

  } else if (page == 3) {
    display.setCursor(0, 0);
    display.println("Deprem Bilgisi:");

    if (depremBuyukluk > 0.0) {
      display.setCursor(0, 10);
      display.print("Mag: ");
      display.print(depremBuyukluk, 1);
      kaydirmaMetni = depremYer;  // Kaydırılacak metin deprem yeri
      yaziyiKaydirVeYaz(depremYer, 20);
    } else {
      display.setCursor(0, 10);
      display.println("Veri yok");
      kaydirmaMetni = "";
    }

  } else if (page == 4) {
    display.setCursor(0, 0);
    display.println("Bilgi Akisi:");
    kaydirmaMetni = bildirim;   // Kaydırılacak metin bilgisayar mesajı
    yaziyiKaydirVeYaz(bildirim, 10);

  } else if (page == 5) {
    display.setCursor(0, 0);
    display.println("Adim Sayisi:");
    display.setCursor(0, 10);
    display.println(adimSayisi);
    kaydirmaMetni = "";  // Adımsayar sabit yazı
  }

  display.display();
}

// ------ HAVA DURUMU ------
void getWeather() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + String(city) + "&appid=" + apiKey + "&units=metric";
    http.begin(client, serverPath);
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      String payload = http.getString();
      DynamicJsonDocument doc(1024);
      if (!deserializeJson(doc, payload)) {
        temperature = doc["main"]["temp"].as<float>();
        weatherDescription = doc["weather"][0]["main"].as<String>();
      }
    }
    http.end();
  }
}

// ------ BİLGİSAYAR MESAJI ------
void getMesaj() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    String serverPath = "http://" + String(mesajServer) + ":" + String(mesajPort) + String(mesajPath);
    http.begin(client, serverPath);
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      String payload = http.getString();
      payload.replace("<p>", "");
      payload.replace("</p>", "");
      bildirim = payload;
    }
    http.end();
  }
}

// ------ TİLT KONTROL ------
void checkTilt() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  float tiltY = a.acceleration.y;
  float tiltX = a.acceleration.x;

  unsigned long now = millis();

  // ---- Eğer uykudaysa sadece sağa eğmeyi kontrol et ----
  if (isSleeping) {
    if (tiltY > 7) {
      if (!tiltRightDetected) {
        tiltRightStart = now;
        tiltRightDetected = true;
      } else if (now - tiltRightStart >= 3000) {
        exitSleepMode();
        tiltRightDetected = false;
      }
    } else {
      tiltRightDetected = false;
    }
    return;
  }

  // ---- Normal modda sayfa değişimi ----
  if (tiltY > 6 && now - lastTiltChange > 1000) {
    page++;
    if (page > 5) page = 0;  // Artık 5 sayfa var
    lastTiltChange = now;
    showPage();
  }

  if (tiltY < -6 && now - lastTiltChange > 1000) {
    if (page == 0) page = 5;
    else page--;
    lastTiltChange = now;
    showPage();
  }

  // ---- Sleep mode'a geçmek için yukarı bakmayı kontrol et ----
  if (tiltX > 7) {
    if (!tiltUpDetected) {
      tiltUpStart = now;
      tiltUpDetected = true;
    } else if (now - tiltUpStart >= 3000) {
      enterSleepMode();
      tiltUpDetected = false;
    }
  } else {
    tiltUpDetected = false;
  }
}

void checkSteps() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Sadece Z ekseni ivmesi (yukarı aşağı hareket)
  float zIvme = a.acceleration.z;

  unsigned long simdi = millis();

  // Eşik değer: Adım algılaması için zIvme 11 m/s^2'yi geçmeli (yaklaşık 1.1g)
  // Debounce: iki adım arasında en az 400ms olmalı
  if (zIvme > 11.0 && simdi - lastStepTime > 400) {
    adimSayisi++;
    lastStepTime = simdi;
  }
}

// ------ GÖZ ANİMASYONU ------
void wakeup_eyes() {
  int left_x = 24;
  int right_x = 84;
  int y = SCREEN_HEIGHT / 2 - eye_height / 2;

  // Gözler kapalı çiz
  display.clearDisplay();
  display.fillRoundRect(left_x, y + eye_height / 2, eye_width, 2, 1, SSD1306_WHITE);
  display.fillRoundRect(right_x, y + eye_height / 2, eye_width, 2, 1, SSD1306_WHITE);
  display.display();
  delay(300);

  // Gözleri yavaşça aç
  for (int h = 2; h <= eye_height; h += 2) {
    display.clearDisplay();
    display.fillRoundRect(left_x, y + eye_height / 2 - h / 2, eye_width, h, 2, SSD1306_WHITE);
    display.fillRoundRect(right_x, y + eye_height / 2 - h / 2, eye_width, h, 2, SSD1306_WHITE);
    display.display();
    delay(100);
  }

  delay(300);

  // --- Göz kırpma ---
  for (int h = eye_height; h >= 2; h -= 2) {
    display.clearDisplay();
    display.fillRoundRect(left_x, y + eye_height / 2 - h / 2, eye_width, h, 2, SSD1306_WHITE);
    display.fillRoundRect(right_x, y + eye_height / 2 - h / 2, eye_width, h, 2, SSD1306_WHITE);
    display.display();
    delay(50);
  }
  for (int h = 2; h <= eye_height; h += 2) {
    display.clearDisplay();
    display.fillRoundRect(left_x, y + eye_height / 2 - h / 2, eye_width, h, 2, SSD1306_WHITE);
    display.fillRoundRect(right_x, y + eye_height / 2 - h / 2, eye_width, h, 2, SSD1306_WHITE);
    display.display();
    delay(50);
  }

  delay(500);
}

// ------ SLEEP MODE ------
void enterSleepMode() {
  isSleeping = true;
  display.clearDisplay();
  display.setCursor(0, 10);
  display.println("Going to sleep...");
  display.display();
  delay(1000);

  int left_x = 24;
  int right_x = 84;
  int y = SCREEN_HEIGHT / 2 - eye_height / 2;

  for (int h = eye_height; h >= 2; h -= 2) {
    display.clearDisplay();
    display.fillRoundRect(left_x, y + eye_height / 2 - h / 2, eye_width, h, 2, SSD1306_WHITE);
    display.fillRoundRect(right_x, y + eye_height / 2 - h / 2, eye_width, h, 2, SSD1306_WHITE);
    display.display();
    delay(50);
  }

  display.clearDisplay();
  display.setCursor(0, 10);
  display.println("Zzz...");
  display.display();
  delay(1000);

  display.ssd1306_command(SSD1306_DISPLAYOFF);
}

void exitSleepMode() {
  display.ssd1306_command(SSD1306_DISPLAYON);
  display.clearDisplay();
  display.setCursor(0, 10);
  display.println("Trying to wake up...");
  display.display();
  delay(2500);

  wakeup_eyes();

  isSleeping = false;
  showPage();
}

void getDeprem() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    // Sadece Türkiye sınırlarındaki son deprem
    String serverPath = "https://earthquake.usgs.gov/fdsnws/event/1/query?format=geojson&minlatitude=36&maxlatitude=42&minlongitude=26&maxlongitude=45&orderby=time&limit=1";

    http.begin(client, serverPath);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String payload = http.getString();

      DynamicJsonDocument doc(8192);
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        depremBuyukluk = doc["features"][0]["properties"]["mag"].as<float>();
        depremYer = doc["features"][0]["properties"]["place"].as<String>();
      }
    }

    http.end();
  }
}
