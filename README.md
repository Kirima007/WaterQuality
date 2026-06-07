# Water Quality Monitoring System (Firmware)

A professional-grade, FreeRTOS-based firmware for an ESP32 water quality monitoring device. Developed by AIOT LAB (KMITL).

This system continuously monitors water quality metrics, displays data via an I2C LCD, and transmits payloads to a remote server using either Wi-Fi or a Cellular GPRS connection. It features an interactive menu system navigated via a rotary encoder and includes robust subsystems for sensor calibration, alarm thresholds, and Over-The-Air (OTA) firmware updates.

## Features

* **Multi-Tasking Architecture**: Utilizes FreeRTOS for decoupled task management (Sensing, GPS, Display, Input, Networking, and Alarms) to ensure system stability.
* **Dual Hardware Configurations**: 
  * 1-Channel Mode: Monitors Salinity (EC) and Temperature.
  * 3-Channel Mode: Monitors Salinity (EC), pH, Dissolved Oxygen (DO), and Temperature.
* **Dual Network Connectivity**: Supports seamless switching between Wi-Fi and SIM800L (GPRS).
* **Hardware User Interface**: 20x4 Character LCD paired with a rotary encoder for navigation, calibration, and local configuration.
* **On-Device Calibration**: Auto and manual calibration for alpha/beta multi-point correction, stored securely in Non-Volatile Storage (NVS).
* **Over-The-Air (OTA) Updates**: Supports remote firmware updates via HTTP over Wi-Fi.
* **Location Tracking**: Integrates Neo-N8M GPS for geospatial data tagging.

## Prerequisites

### Hardware
* ESP32 Development Board
* Adafruit ADS1115 (16-bit ADC)
* 20x4 I2C LCD Display
* Rotary Encoder (with push button)
* DS18B20 One-Wire Temperature Sensor
* Analog Sensors (EC, pH, DO)
* SIM800L GSM/GPRS Module
* Neo-N8M GPS Module
* Buzzer & RGB LED for alarm states

### Software
* [PlatformIO IDE](https://platformio.org/) (VS Code Extension)
* C++ / Arduino Framework for ESP32

## Installation & Build Instructions

1. **Clone the repository**
   ```bash
   git clone https://github.com/Kirima007/WaterQuality.git
   cd WaterQuality
   ```

2. **Open with PlatformIO**
   Open the project folder in VS Code with the PlatformIO extension installed. PlatformIO will automatically download the required libraries specified in `platformio.ini`.

3. **Select Environment**
   The `platformio.ini` file contains two build environments based on your hardware setup:
   * `env:Sensor_1` (1-Channel mode)
   * `env:Sensor_3` (3-Channel mode)

4. **Build and Upload**
   Connect your ESP32 via USB and run the upload command:
   ```bash
   pio run -e Sensor_1 -t upload
   ```

## Configuration

Core system parameters, network credentials, and API endpoints are defined in `src/config.h`. Modify this file before compiling for production deployment:

```c
#define HTTP_HOST       "xxx.xxx.xxx.xxx"
#define HTTP_PATH       "/api/data"
#define WIFI_SSID       "BCK-WIFI"
#define WIFI_PASS       "123456789"
#define SIM_APN         "internet"
```

*Note: Runtime configurations such as Network Mode (Wi-Fi/SIM), Device ID, Alarm Limits, and Calibration offsets are stored in NVS and can be modified directly through the device's physical menu.*

## Usage Overview

Once powered on, the device enters the `STARTUP` state and initializes hardware. The user can interact with the system using the rotary encoder:
* **Rotate**: Scroll through menu items or adjust values.
* **Short Press**: Select an item, confirm a value, or navigate forward/backward.
* **Long Press**: Trigger special actions (e.g., capture stable calibration values, force manual data sync).

**Main Menu Structure:**
* Monitor Data (View real-time sensor metrics and network/GPS status)
* Network Status (Check Wi-Fi RSSI or SIM CSQ)
* GPS Status (View Latitude, Longitude, and Satellite locks)
* Setting (Switch between Wi-Fi/SIM and toggle Buzzer)
* Sensor Calibrate (Auto/Manual calibration for specific sensors)
* Temp Calibrate (Set offset for DS18B20)
* Alarm Limits (Configure Green/Yellow/Red LED and Buzzer thresholds)
* Firmware Update (Check and download OTA updates)
* System Info (View Device ID, Firmware version, and Factory Reset option)

## License

**Proprietary License**  
Copyright (c) 2026 AIOT LAB (KMITL). All rights reserved.

This software is the confidential and proprietary information of AIOT LAB, King Mongkut's Institute of Technology Ladkrabang (KMITL). You shall not disclose such Confidential Information and shall use it only in accordance with the terms of the license agreement you entered into with AIOT LAB.
