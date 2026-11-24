# Water Quality Monitoring System — Firmware

FreeRTOS firmware for an ESP32-based field water-quality monitor, developed by **AIOT LAB, KMITL**.

The device measures salinity (EC), pH, dissolved oxygen (DO) and water temperature. It shows live readings on a 20×4 LCD and reports them, with GPS coordinates, to a backend server over **Wi-Fi** or **2G GPRS (SIM800L)**. Calibration, alarm limits, network mode and firmware updates are all handled on the device using a rotary encoder.

---

## Contents

- [Features](#features)
- [Hardware](#hardware)
- [Firmware Architecture](#firmware-architecture)
- [Getting Started](#getting-started)
- [Operation](#operation)
- [Calibration](#calibration)
- [Server Interface](#server-interface)
- [Over-the-Air Updates](#over-the-air-updates)
- [Troubleshooting](#troubleshooting)
- [License](#license)

---

## Features

| Area | Description |
|---|---|
| **Sensing** | 16-bit ADC (ADS1115) with 10-sample averaging, plus DS18B20 temperature compensation |
| **Variants** | Two builds from one code base: 1-channel (salinity) and 3-channel (salinity, pH, DO) |
| **Connectivity** | Wi-Fi or SIM800L GPRS, selectable at runtime from the menu |
| **Location** | GPS position attached to every data upload |
| **User interface** | 20×4 I2C LCD, rotary encoder with short / long press, buzzer and RGB status LED |
| **Calibration** | Two-point linear calibration with a live stability indicator; results stored in NVS |
| **Alarms** | Per-parameter green / yellow / red thresholds with LED and buzzer patterns |
| **OTA** | HTTP firmware update with MD5 verification and A/B partitions |
| **Robustness** | Task watchdog, LCD/ADC hot-plug detection, Wi-Fi auto-reconnect, modem auto-recovery |

---

## Hardware

### Bill of Materials

| Component | Notes |
|---|---|
| LilyGO **T-Call SIM800L** (v1.4) | ESP32 + SIM800L + IP5306 power management |
| ADS1115 16-bit ADC | I2C `0x48` |
| 20×4 character LCD with PCF8574 backpack | I2C `0x27` |
| DS18B20 waterproof probe | 1-Wire, with 4.7 kΩ pull-up |
| GPS module (u-blox NEO series) | UART, 9600 baud |
| Analog EC / pH / DO sensor boards | Read by ADS1115 channels A0 / A2 / A1 |
| Rotary encoder with push button | |
| Buzzer and RGB LED | |
| 18650 Li-ion cells | See [Power Supply](#power-supply) |

### Pin Map

| Function | GPIO | | Function | GPIO |
|---|---|---|---|---|
| I2C SDA / SCL | 21 / 22 | | Encoder CLK / DT / SW | 32 / 33 / 14 |
| 1-Wire (DS18B20) | 18 | | Buzzer | 2 |
| GPS RX / TX | 19 / 25 | | RGB LED (3-ch) R / G / B | 13 / 12 / 15 |
| Modem RX / TX | 26 / 27 | | RGB LED (1-ch) R / G / B | 9 / 13 / 12 |
| Modem PWRKEY / RST / POWER | 4 / 5 / 23 | | | |

> [!WARNING]
> **GPIO 9** (1-channel RGB red) is connected internally to the SPI flash. Move it to a free pin in new hardware revisions. **GPIO 12** is a boot-strapping pin and must be LOW at reset.

### Power Supply

The SIM800L draws current bursts of up to **2 A** while transmitting. If the supply sags during a burst, the ESP32's flash reads fail. This shows up as random resets, `Checksum failed` at boot, or `IllegalInstruction` panics, while a multimeter still reads a normal voltage.

- Use cells and wiring that can deliver at least 2 A peaks with minimal voltage drop.
- Place a low-ESR bulk capacitor (1000–2200 µF) close to the modem supply.
- Do not route the modem supply through an undersized boost converter.
- The LCD backpack and ADS1115 modules usually pull up the I2C lines to their own supply. If they run from 5 V, use an I2C level shifter so 5 V never reaches the ESP32's 3.3 V pins.

---

## Firmware Architecture

The firmware is a set of FreeRTOS tasks that talk only through queues and a central state machine.

| Task | Core | Priority | Stack | Responsibility |
|---|:-:|:-:|--:|---|
| `Sensor` | 0 | 4 | 4 KB | Read ADS1115 and DS18B20 every 1 s and publish `SensorData` |
| `Alarm` | 0 | 3 | 2 KB | Compare readings against thresholds; drive the LED and buzzer |
| `Sound` | 0 | 2 | 1 KB | Play UI sound effects from a queue |
| `SIM` | 0 | 2 | 8 KB | GPRS connection, data upload, modem recovery |
| `WiFi` | 0 | 2 | 8 KB | Wi-Fi connection, data upload, OTA |
| `GPS` | 0 | 1 | 4 KB | Parse NMEA and publish `GPSData` |
| `Rotary` | 1 | 4 | 2 KB | Encoder and button debounce; emit `ButtonEvent`s |
| `Input` | 1 | 3 | 4 KB | Route events to the `StateMachine` |
| `Screen` | 1 | 2 | 8 KB | Render the current state to the LCD |

### Project Layout

```
├── include/              Public headers (task interfaces, shared types)
├── src/
│   ├── main.cpp          Task creation, watchdog setup
│   ├── Config.template.h Template for local configuration (copy to Config.h)
│   ├── SensorTask.cpp    Acquisition loop
│   ├── SensorMath.cpp    Conversion, temperature compensation, stability tracker
│   ├── StateMachine_*.cpp  Menu and workflow logic (1CH / 3CH)
│   ├── ScreenManager_*.cpp LCD rendering (1CH / 3CH)
│   ├── NVSManager.cpp    Persistent settings and calibration
│   ├── WifiTask.cpp      Wi-Fi uplink and OTA
│   └── SimTask.cpp       GPRS uplink
├── docs/                 Engineering notes (e.g. reset analysis)
└── platformio.ini        Build environments
```

---

## Getting Started

### Prerequisites

- [Visual Studio Code](https://code.visualstudio.com/) with the [PlatformIO](https://platformio.org/) extension
- USB driver for the CP210x serial bridge

### 1. Clone

```bash
git clone https://github.com/Kirima007/WaterQuality.git
cd WaterQuality
```

### 2. Configure

Copy the template and edit it to match your deployment:

```bash
cp src/Config.template.h src/Config.h
```

```c
#define HTTP_HOST   "your.server.address"
#define HTTP_PORT   80
#define WIFI_SSID   "your_ssid"
#define WIFI_PASS   "your_password"
#define SIM_APN     "internet"        // DTAC: "internet", True: "true", AIS: "ais"
#define DEVICE_ID   99                // default ID; can be changed on the device
#define FW_VERSION  "V1.5A"
```

> [!IMPORTANT]
> `src/Config.h` holds credentials and is listed in `.gitignore`. Never commit it.

### 3. Build and Upload

Select the environment for your hardware:

| Environment | Use |
|---|---|
| `Sensor_1` | 1-channel device (salinity + temperature) |
| `Sensor_3` | 3-channel device (salinity, pH, DO + temperature) |
| `Sensor_3_nomodem` | Diagnostic build: 3-channel with the SIM800L kept powered off |

```bash
pio run -e Sensor_3 -t upload
pio device monitor -b 115200
```

If the upload fails with `Wrong boot mode detected (0x13)`, hold **GPIO0 to GND**, press **RST**, and upload again.

---

## Operation

### Controls

| Input | Action |
|---|---|
| Rotate | Move between menu items or change a value |
| Short press | Select or confirm |
| Long press (1.5 s) | Context action, e.g. save a calibration point or send data now |

### Menu

| Menu | Purpose |
|---|---|
| Monitor Data | Live readings with network and GPS status |
| Network Status | Wi-Fi RSSI or GSM signal quality (CSQ) |
| GPS Status | Latitude, longitude and satellite count |
| Setting | Network mode (Wi-Fi / SIM) and buzzer mute |
| Sensor Calibrate | Auto or manual calibration for each sensor |
| Temp Calibrate | DS18B20 offset |
| Alarm Limits | Green / yellow / red thresholds per parameter |
| Firmware Update | Check for and install OTA updates |
| System Info | Device ID, firmware version, factory reset |

All runtime settings live in NVS and are kept across reboots and firmware updates. On first boot, missing keys are filled with safe defaults.

---

## Calibration

Each sensor uses a two-point linear correction on top of its temperature-compensated base value:

```
value = α · base(V, T) + β
α = (t₂ − t₁) / (m₂ − m₁)        β = t₁ − α · m₁
```

Here *t* is the known value of the standard solution and *m* is the value the device measured in it. For pH, the standards are **4.01** and **6.86**.

### Procedure

1. Rinse the probe and immerse it in the first standard solution.
2. The calibration screen shows the live value, the raw voltage and the temperature, plus a stability indicator:
   - `WAIT n/15` — the window is still collecting samples
   - `WAIT ±x.xx` — the reading is still drifting
   - `** STABLE **` — the window has settled
3. When the screen shows **STABLE**, long-press to capture the point.
4. Repeat with the second standard. The device computes α and β and saves them to NVS.
5. Press to cancel at any time. The previous calibration is kept.

The stability window is the last 15 samples (about 15 s). A reading counts as stable when its spread is at most **0.03 pH**, or **0.5 units** for the other parameters.

---

## Server Interface

Readings are sent as an HTTP `POST` of JSON to `HTTP_PATH`:

```json
{
  "id": 3,
  "temp": 28.4,
  "salinity": 12.35,
  "ph": 7.02,
  "o2": 6.80,
  "address": { "x": "13.729900", "y": "100.775800" }
}
```

`ph` and `o2` are sent by 3-channel devices only. Calibration events are posted to `/api/calibrate/{salinity|ph|o2}`.

---

## Over-the-Air Updates

The device fetches `HTTP_PATH_OTA_VERSION`, which must return:

```json
{
  "latest_version": "V1.6",
  "firmware_url": "http://your.server.address/ota/3/fw/firmware.bin",
  "md5": "d41d8cd98f00b204e9800998ecf8427e"
}
```

`firmware_url` must be a full `http://` URL. The device keeps only its path and downloads it from `HTTP_HOST`:`HTTP_PORT`. When `latest_version` differs from `FW_VERSION`, the user can confirm the update from the **Firmware Update** menu. The image is written to the inactive OTA slot (`min_spiffs.csv`, 2 × 1.9 MB), checked against the MD5 if one is given, and booted on the next restart. OTA currently runs over Wi-Fi only.

---

## Troubleshooting

| Symptom | Likely cause | Action |
|---|---|---|
| Random resets, `Checksum failed`, `IllegalInstruction` | Supply sag during SIM800L transmit bursts | See [Power Supply](#power-supply); test with `Sensor_3_nomodem` |
| `rst:0xf (BROWNOUT_RESET)` | Battery depleted or wiring resistance too high | Charge or replace the cells; shorten the power leads |
| LCD blank, everything else running | LCD not detected at `0x27` | Check wiring; the LCD is re-detected every 3 s |
| All readings show 0 | ADS1115 not detected | Check the I2C wiring; the ADC is re-detected every 10 s |
| Upload fails: `Wrong boot mode (0x13)` | Auto-reset circuit not entering download mode | Hold GPIO0 to GND while resetting |

To read a crash, open the serial monitor at 115200 baud and decode the backtrace:

```bash
pio device monitor -b 115200 --filter esp32_exception_decoder
```

The full root-cause investigation of the field resets is in [`docs/RESET_ANALYSIS.md`](docs/RESET_ANALYSIS.md).

---

## License

**Proprietary.** Copyright © 2026 AIOT LAB, King Mongkut's Institute of Technology Ladkrabang (KMITL). All rights reserved.

This software is confidential and proprietary to AIOT LAB, KMITL. Do not use, copy, modify or distribute it except under the terms of a license agreement with AIOT LAB.
