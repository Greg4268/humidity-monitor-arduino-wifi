# Arduino Smart Humidity Monitor & Alert System

> *An Internet-connected humidity and temperature monitoring system built with Arduino Uno R4 WiFi, the Adafruit Si7021 sensor, and a cloud-based alert dashboard. Perfect for terrariums, indoor plant environments, or home humidity control.*

---

<p align="center">
   <img src="./assets/tinker.png" alt="arduino" width="600"/>
   <img src="./assets/pic.jpeg" width="600">
</p>
**Note that TinkerCAD diagram is missing the Si7021 humidity module**

---

## About

### What problem does this solve?
Maintaining proper humidity levels is critical for both specialized environments like terrariums and everyday living spaces. This project provides a comprehensive solution for:

- **Terrarium environments:** Ensuring optimal conditions for plants and small creatures
- **Indoor spaces:** Preventing mold growth while maintaining comfortable living conditions
- **Remote monitoring:** Checking environmental conditions from anywhere via web dashboard

This smart monitor provides real-time local feedback through LCD, LEDs, and buzzers, while also sending alerts to a cloud dashboard that can be accessed from any web browser.

### How it works
The system creates a complete monitoring loop:

1. **Sensor data collection:** The Si7021 sensor measures humidity and temperature with high precision
2. **Local feedback:** LCD display, LEDs, and buzzer provide immediate status information
3. **WiFi connectivity:** Arduino Uno R4 WiFi connects to your home network
4. **Cloud alerts:** Status updates are sent to a Flask web server hosted on Railway

---

## Features

- [x] **Live Environmental Monitoring** — Continuous humidity and temperature readings with 2 decimal place precision
- [x] **Dual Operating Modes**
  - Terrarium mode (60-93% ideal humidity range)
  - Indoor mode (30-60% ideal humidity range)
- [x] **Multi-Level Local Alert System**
  - Green LED: Ideal humidity range
  - Yellow LED + gentle tone: Approaching range limits
  - Red LED + warning tone: Outside acceptable range
- [x] **Informative LCD Display** — Shows current readings and status messages
- [x] **Self-Maintaining Sensor** — Automatic heater cycling to prevent condensation and maintain accuracy
- [x] **WiFi Connectivity** — Connects to your home network for cloud integration
- [x] **Web Dashboard** — Access your system's status from anywhere
  - Password-protected interface
  - Real-time status updates
  - Timestamp for last alert received
- [x] **Resilient Connections** — Automatic reconnection to WiFi and server if connectivity is lost
- [x] **RESTful API** — Endpoints for programmatic access to alert data
- [ ] **TODO: Alert History** — Future feature to store and display historical humidity data and alerts

---

## How It Works

The system operates as a complete monitoring loop:

1. **Data Collection:**
   - The Si7021 sensor takes humidity and temperature readings every 5 seconds
   - Sensor performs periodic self-cleaning using the built-in heater function

2. **Local Processing:**
   - Arduino compares readings against the configured thresholds
   - Status is determined (GOOD, WARNING_APPROACHING, or WARNING_BAD)
   - Local feedback is provided through LCD, LEDs, and buzzer

3. **Cloud Communication:**
   - Status updates are sent to the web server when:
     - Status changes from previous reading
     - At regular intervals (every 5 minutes)
     - On initial boot
   - JSON data is transmitted via HTTPS POST requests

4. **Web Interface:**
   - Flask server receives and processes updates
   - Web dashboard displays current status with visual indicators
   - Password protection ensures only authorized users can view data

---

## Getting Started

### Hardware Requirements

- Arduino Uno R4 WiFi board
- Adafruit Si7021 Temperature & Humidity Sensor
- 16x2 LCD Display 
- 3x LEDs (red, yellow, green)
- Piezo buzzer
- 2x 220Ω Resistors (for LEDs)
- 10kΩ Potentiometer (for LCD contrast)
- Breadboard and jumper wires
- Micro USB cable (for programming and powering Arduino)

<p align="center">
   <img src="./assets/schematic.png" alt="arduino" width="600"/>
</p>
** Please note that the Si7021 Humidity & Temperature sensor is not in this schematic ** 

see readme content for pin connections

### Software Requirements

- Arduino IDE (2.0 or newer recommended)
- Required Arduino libraries:
  - Adafruit_Si7021
  - Adafruit_BusIO
  - LiquidCrystal
  - WiFiS3
  - ArduinoJson
- Web server (Flask application running on Railway or similar service)
- Web browser to access dashboard

### Hardware Setup

1. **LCD Display Connection:**
   - RS pin to Arduino pin 12
   - EN pin to Arduino pin 11
   - D4 pin to Arduino pin 5
   - D5 pin to Arduino pin 4
   - D6 pin to Arduino pin 3
   - D7 pin to Arduino pin 2
   - Connect potentiometer to LCD contrast pin
   
2. **LED Setup:**
   - Red LED to Arduino pin 10 (with 220Ω resistor)
   - Yellow LED to Arduino pin 9 (with 220Ω resistor)
   - Green LED to Arduino pin 8 (with 220Ω resistor)
   
3. **Buzzer Connection:**
   - Connect to Arduino pin 6
   
4. **Si7021 Sensor Connection:**
   - Connect VCC to 3.3V
   - Connect GND to ground
   - Connect SCL to Arduino SCL pin (A5)
   - Connect SDA to Arduino SDA pin (A4)

### Software Installation

#### Arduino Code Setup:

1. Install the Arduino IDE
2. Install required libraries through the Library Manager:
   - Adafruit Si7021 Library
   - Adafruit BusIO
   - LiquidCrystal
   - WiFiS3
   - ArduinoJson
3. Clone or download the project code
4. Open the .ino file in Arduino IDE
5. Configure your WiFi credentials in the code:
   ```cpp
   char ssid[] = "YourWiFiNetwork";  // Replace with your WiFi name
   char pass[] = "YourWiFiPassword";  // Replace with your WiFi password
   ```
6. Set your web server details:
   ```cpp
   const char* server = "your-app-name.up.railway.app";  // Your Railway app URL
   const int port = 443;  // HTTPS port
   const char* path = "/update-alert";  // API endpoint path
   ```
7. Upload the code to your Arduino Uno R4 WiFi

#### Web Server Setup:

1. Clone the Flask server repository
2. Deploy to Railway:
   - Connect your GitHub repository
   - Set environment variables:
     - `AUTH_USERNAME`: Dashboard access username
     - `AUTH_PASSWORD`: Dashboard access password
   - Deploy the application
3. Note your application URL (will be something like `your-app-name.up.railway.app`)

---

## Usage

### Local Operation

1. After uploading the code and connecting to power, the system will:
   - Display "Humidity Monitor" and "Starting..." on LCD
   - Connect to your WiFi network (watch Serial Monitor for connection status)
   - Begin monitoring humidity and temperature
   
2. The LCD will display current readings and status based on humidity levels

3. LEDs and buzzer provide immediate feedback:
   - Green LED: Humidity is in ideal range
   - Yellow LED + beep: Humidity is approaching limits
   - Red LED + warning tone: Humidity is outside acceptable range
   
4. Open the Serial Monitor (115200 baud) to view detailed readings and connection status

5. To switch between indoor and terrarium modes, modify this line in the code:
   ```cpp
   bool measureIndoor = true;  // Set to false for terrarium mode
   ```

### Remote Monitoring

1. Access your web dashboard by visiting your Railway app URL
2. Enter the username and password you configured in environment variables
3. View the current humidity status and last update timestamp
4. The dashboard will automatically reflect status changes without needing to refresh

---

## Configuration

### Arduino Configuration

The system has predefined humidity ranges for both terrarium and indoor environments:

#### Terrarium Mode:
- Ideal range: 65-88% humidity
- Warning range: 60-65% or 88-93% humidity
- Alert range: <60% or >93% humidity

#### Indoor Mode:
- Ideal range: 35-55% humidity
- Warning range: 30-35% or 55-60% humidity
- Alert range: <30% or >60% humidity

To adjust these ranges, modify the enum values in the code:

```cpp
// For terrarium environments
enum terrarium {
  TERRARIUM_LOW = 60, 
  TERRARIUM_CLOSE_LOW = 65, 
  TERRARIUM_CLOSE_HIGH = 88, 
  TERRARIUM_HIGH = 93
};

// For indoor environments
enum indoor {
  INDOOR_LOW = 30, 
  INDOOR_CLOSE_LOW = 35, 
  INDOOR_CLOSE_HIGH = 55, 
  INDOOR_HIGH = 60
};
```

### Web Server Configuration

The Flask server can be configured using environment variables:

- `AUTH_USERNAME`: Username for dashboard access
- `AUTH_PASSWORD`: Password for dashboard access
- `PORT`: (Optional) Port number for the server (defaults to 8080)

---

## Web Dashboard

The web dashboard provides a simple interface to monitor your system's status from anywhere:

### Features:

- **Real-time status display**: Shows current humidity condition
- **Status indicators**: 
  - 🟢 GOOD: Humidity is in ideal range
  - 🟡 WARNING (APPROACHING BAD): Humidity is approaching limits
  - 🔴 WARNING (BAD): Humidity is outside acceptable range
- **Timestamp**: Shows when the last update was received
- **Password protection**: Secure access to your environmental data

### API Endpoints:

- **GET /ping**: Simple health check endpoint
- **GET /get-alert**: Returns current alert status as JSON
- **POST /update-alert**: Endpoint for Arduino to send status updates
  - Requires JSON body with `status` field
  - Valid status values: "good", "warning(approaching bad)", "warning(bad)"

---

## 🖼️ Project Images
<img src="assets/pic.jpeg" alt="arduino" width="600"/>
<img src="./assets/2.jpeg" width="600">


<img src="./assets/goodPage.png" alt="webpage of good state" width="600">
<img src="./assets/criticalPage.png" alt="webpage of bad state" width="600">

---

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.
