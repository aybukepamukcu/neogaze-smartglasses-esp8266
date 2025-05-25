# 🕶️ NeoGaze- The Smart Glasses Powered by ESP8266   
**NeoGaze** is a lightweight smart glasses system powered by the ESP8266 microcontroller, designed to bring real-time information directly to your eyes — no phone, no hands, no distractions.

This wearable device features a compact OLED display that shows current weather conditions, exchange rates, earthquake alerts, step count, and even messages from a local web server. It combines multiple sensor inputs, including motion detection with an MPU6050, to enable intuitive **tilt-based gesture navigation**.

Instead of using buttons or touch controls, pages on the OLED display are changed simply by tilting your head. Looking upward for 3 seconds puts the device into **sleep mode**, while a right tilt wakes it up — making NeoGaze both energy-efficient and naturally interactive.

From the minimalist UI to sensor-based control, NeoGaze represents a fusion of embedded systems, IoT, and wearable design — built entirely using open-source libraries, APIs, and modular components.

Feel free to replicate this project using just breadboards and standard components — no soldering or custom PCBs necessary. NeoGaze is fully open-source and beginner-friendly!

Below are detailed images and hardware breakdowns from the project.

<img src="https://github.com/user-attachments/assets/ba2a3fb8-bd45-462b-b1d2-8294abe628a5" width="500">
<img src="https://github.com/user-attachments/assets/6e95a37a-0b00-4520-97b3-6f35cae7d86a" width="500">
<img src="https://github.com/user-attachments/assets/a21d1e54-0221-4971-8c4b-9a4db89bb5c0" width="400">
<img src="https://github.com/user-attachments/assets/ed513cf5-cb33-4193-9e09-407e80f78fd4" width="400">


### 🔍 Overview
This project is a smart glasses system built with ESP8266, featuring an OLED display and MPU6050 IMU. It fetches real-time data over Wi-Fi and displays it in a minimal and animated interface.

### 🎯 Features
- ⏰ Real-time clock and date via NTP
- 🌦 Weather data using OpenWeatherMap API
- 💱 Currency rates using ExchangeRate API
- 🌍 Earthquake info from USGS
- 📩 Message display from local web server
- 💤 Sleep mode triggered by upward tilt
- 👁️ Wake-up and blink animation
- 👣 Step counting using Z-axis acceleration
- 🤸 Page switching via tilt gesture

  
### 🔧 How It Works   

On boot, the ESP8266 connects to your Wi-Fi network and synchronizes the current time using an NTP server.

It queries the OpenWeatherMap API for weather data and the ExchangeRate API for currency rates.

It fetches the latest earthquake activity in your region using the USGS open API (no key required).

It also requests a custom message from a local web server via HTTP and displays it as a scrolling banner.

The MPU6050 motion sensor detects Z-axis movement for step counting and Y/X-axis tilt for page navigation.

Pages on the OLED screen are switched by tilting your head left or right.

Looking upward for 3 seconds activates sleep mode, turning off the display to save power.

Tilting right for 3 seconds wakes the device up and triggers a smooth eye-opening animation.

All system information is displayed on a 128x32 OLED screen in a compact and scrollable interface.

### 🧰 Hardware Used

- NodeMCU (ESP8266)
- OLED Display (0.91”, I2C, 128x32)
- MPU6050 Accelerometer & Gyroscope
- Slide Switch
- Li-Po Battery (3.7V)
- MT3608 Step-up Converter
- TP4056 Charging Module (HW-107)
- Pull-up Resistors (10k ohms)
- Connecting Wires
- 3D Printed Frame
- Perfboard
- Mirrors
- Sunglasses

### 📦 Required Libraries

Install the following libraries via **Arduino Library Manager**:

- **Adafruit GFX** — for graphics primitives and drawing support on OLED  
- **Adafruit SSD1306** — for controlling the OLED display (128x32)  
- **Adafruit MPU6050** — for interacting with the motion sensor  
- **Adafruit Unified Sensor** — dependency used by MPU6050  
- **ESP8266WiFi** — built-in library for Wi-Fi communication  
- **ESP8266HTTPClient** — for making HTTP requests (e.g., API calls)  
- **ArduinoJson** — for parsing JSON responses from APIs  
- **time** — for NTP time synchronization

### ⚙️ Setup
#### 1. Add your Wi-Fi credentials:
```cpp
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
```
🔐 These values are required to connect your ESP8266 device to your home or mobile Wi-Fi network.  
Make sure your device is within range and the SSID/password are typed correctly.     
#### 2. Get your OpenWeatherMap API Key:   
-Visit https://openweathermap.org/api     
-Create a free account    
-Go to API keys section    
-Copy your key and paste it here    
 This key is used to fetch real-time weather data shown on the OLED screen.    
```cpp
const char* apiKey = "YOUR_API_KEY";
```
#### 3. Add your local message server information:
```cpp
const char* mesajServer = "YOUR_SERVER_IP";
const int mesajPort = YOUR_PORT;
const char* mesajPath = "/your_path.html";
```
💡 This setup assumes that you are hosting a simple .html file on a local web server (e.g. Python HTTP server, Node.js, or ESP8266 WebServer) within the same Wi-Fi network.
For example, if you run a server at http://192.168.1.50:8000/message.html, you should set:
```cpp
const char* mesajServer = "192.168.1.50";
const int mesajPort = 8000;
const char* mesajPath = "/message.html";

```
#### 4. Set your Exchange Rate API URL:   
-Visit https://www.exchangerate-api.com/   
-Sign up for a free account   
-Get your API key and paste it in place of YOUR_API_KEY (Replace YOUR_API_KEY with your actual key)   
```cpp
String serverPath = "https://v6.exchangerate-api.com/v6/YOUR_API_KEY/latest/TRY";
```
🔁 In this project, the base currency is set to TRY (Turkish Lira).
If you're using a different local currency (e.g. USD, EUR), make sure to replace TRY with your desired currency code.


⚠️ International Users — Modify These Parts of the Code   
Earthquake Data (USGS API) --> The project uses data from the official USGS Earthquake API. No API key is required.   
The default API URL in the code is already filtered to show earthquakes in Turkey. 
```cpp
https://earthquake.usgs.gov/fdsnws/event/1/query?format=geojson&minlatitude=36&maxlatitude=42&minlongitude=26&maxlongitude=45&orderby=time&limit=1
```
You should replace the latitude and longitude ranges with values that match your own region.  
Use a map or coordinates website to find your country's approximate bounding box.  
Example for California, USA:  
```cpp
minlatitude=32&maxlatitude=42&minlongitude=-125&maxlongitude=-114
```
### 📄 License

This project is licensed under the **Creative Commons Attribution-NonCommercial 4.0 International License**.  
You may remix, adapt, and build upon this work non-commercially, and although your new works must also acknowledge the creator, you don’t have to license them on the same terms.

🔗 [View License](https://creativecommons.org/licenses/by-nc/4.0/)

### 👤 About the Creator

Developed by Aybüke Pamukçu.  
If you have questions, suggestions, or want to share your own build, feel free to open an issue or contact me through GitHub.
