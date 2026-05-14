# 🌊 WaterQuality Monitoring System (ESP32)

A robust, real-time water quality monitoring system built on the **ESP32** platform. This project features high-precision sensor reading, GPS tracking, and dual-mode data transmission (WiFi/GPRS) designed for environmental monitoring and IoT applications.

---

## 🚀 Key Features

*   **Multi-Channel Support:** Dynamic support for 1-Channel (Salinity) or 3-Channel (Salinity, pH, DO) sensor configurations via build environments.
*   **High-Precision Sensing:** Uses the **ADS1115 (16-bit ADC)** for accurate analog readings with temperature compensation.
*   **Dual Connectivity:** Flexible data reporting via **SIM800L (GSM/GPRS)** or **WiFi**, configurable through the system menu.
*   **GPS Integration:** Automatically logs Latitude/Longitude coordinates with every sensor data point.
*   **Interactive UI:** Large 20x4 LCD interface with a rotary encoder for menu navigation and system configuration.
*   **On-Device Calibration:** Built-in calibration wizard for DI Water and Salt Solutions, saving values directly to Non-Volatile Storage (NVS).
*   **Real-time Multitasking:** Powered by **FreeRTOS**, ensuring seamless sensor sampling, UI responsiveness, and background data transmission.

---

## 🛠 Hardware Architecture

### Core Components
*   **Microcontroller:** ESP32 (Tested on TTGO T-Call V1.4)
*   **Sensors:** 
    *   **ADS1115:** 16-bit I2C ADC for water quality sensors.
    *   **DS18B20:** OneWire temperature sensor.
    *   **GPS Module:** Serial NEO-6M or compatible.
*   **Communication:**
    *   **SIM800L:** GSM/GPRS modem for remote logging.
    *   **WiFi:** Integrated ESP32 WiFi for local hotspot connectivity.
*   **User Interface:**
    *   **LCD 20x4:** I2C display.
    *   **Rotary Encoder:** Navigation (CLK, DT, SW).
    *   **Buzzer & RGB LED:** Status and alarm notifications.

### Pin Mapping (Default)
| Component | Pin | Function |
| :--- | :--- | :--- |
| **Rotary Encoder** | 32, 33, 14 | CLK, DT, SW |
| **OneWire (Temp)** | 18 | DS18B20 Data |
| **GPS** | 19 (RX), 25 (TX) | Serial Communication |
| **Buzzer** | 2 | Alarm Output |
| **I2C (LCD/ADC)** | 21 (SDA), 22 (SCL) | Bus Communication |
| **RGB LED** | 15, 13, 12 | R, G, B Status (3-CH mode) |

---

## 💻 Software Architecture

The system utilizes a **Multi-tasking architecture** based on FreeRTOS to handle complex operations without blocking the main logic:

*   **SensorTask (Core 0):** High-frequency sampling of ADC and Temperature data.
*   **GPSTask (Core 0):** Asynchronous parsing of NMEA strings from the GPS module.
*   **Network Tasks (Core 0):** Independent tasks for SIM800L and WiFi state management and HTTP POST requests.
*   **ScreenManager (Core 1):** Dedicated UI rendering and LCD updates.
*   **StateMachine (Core 1):** Central logic handling user inputs, menu navigation, and system states.

---

## 📦 Getting Started

### 1. Prerequisites
*   Visual Studio Code with **PlatformIO IDE** extension.
*   ESP32 Development Board.

### 2. Configuration
Modify `src/Config.h` to match your deployment environment:
```cpp
#define DEVICE_ID       99
#define SIM_APN         "internet"        // APN (e.g., "internet", "ais", "true")
#define WIFI_SSID       "Your_SSID"
#define WIFI_PASS       "Your_Password"
#define HTTP_HOST       "your.api.endpoint"
```

### 3. Build & Flash
Choose the environment based on your hardware setup:

*   **1-Channel Mode:** `pio run -e Sensor_1 -t upload`
*   **3-Channel Mode:** `pio run -e Sensor_3 -t upload`

---

## 📖 User Guide

### Navigation
*   **Rotate Encoder:** Navigate through menus or change values.
*   **Short Press:** Confirm selection or enter a menu.
*   **Long Press:** Exit or go back to the previous screen.

### Calibration Procedure
1.  Enter **Calibration Mode** from the Main Menu.
2.  Follow the **DI Water** prompt: Submerge the sensor in distilled water and wait for the reading to stabilize.
3.  Follow the **Salt Solution** prompt: Submerge the sensor in the standard salt solution.
4.  Confirm and save: The system will calculate the slope and offset and store them in the NVS.

### Data Reporting
Data is automatically packaged into a JSON payload and sent to the configured `HTTP_HOST`:
```json
{
  "id": 99,
  "salinity": "12.85",
  "temp": "28.5",
  "address": {
    "x": "13.123456",
    "y": "100.123456"
  }
}
```

---

## 🏷️ Credits
**Developed by:** Weerapat C. (2026)  
**Laboratory:** AIOT LAB  
**Version:** V1.2(A)
