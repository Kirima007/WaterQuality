# WaterQuality

โปรเจกต์ WaterQuality เป็นระบบวัดความเค็มน้ำและส่งข้อมูลผ่าน SIM/GPRS บนบอร์ด ESP32
โดยออกแบบสำหรับบอร์ดที่มีโมดูล SIM800L ในตัว เช่น TTGO T-Call V1.4

## ฟีเจอร์หลัก

- วัดค่าความเค็ม (PPT) โดยใช้ ADS1115 และสูตรปรับ calibrate
- วัดอุณหภูมิด้วย DS18B20
- จอ LCD 20x4 แสดงผลข้อมูลและเมนู
- เมนูควบคุมด้วย rotary encoder พร้อมปุ่มกด
- เชื่อมต่อ GPRS ด้วย SIM800L และส่งข้อมูล HTTP POST ไปยังเซิร์ฟเวอร์
- เชื่อมต่อ Wi-Fi และส่งข้อมูล HTTP POST ไปยังเซิร์ฟเวอร์
- มีหน้า System Info แสดง Device ID, Sensor CH และข้อความ `by AIOT LAB`
- มีการแจ้งเตือน LED/Buzzer เมื่อค่าความเค็มอยู่ในระดับเตือนหรืออันตราย
- Calibration ด้วยน้ำ DI และสารละลายน้ำที่มีค่า EC เป้าหมาย

## เมนูหลัก

- Read Temp
- Read GPS
- LED Threshold
- Calibrate Mode
- System Info
- Back

## Calibration

โหมด Calibrate มีสองขั้นตอนหลัก:

1. Calibrate DI Water
   - target EC = `1.413 us/cm`
2. Calibrate Salt Solution
   - target EC = `12.88 ms/cm`

หลัง calibration โปรแกรมจะคำนวณค่า `alpha` และ `beta` และเก็บใน NVS

## การเชื่อมต่อเครือข่าย

ระบบรองรับสองโหมดการส่งข้อมูล:

### โหมด SIM/GPRS
- ใช้โมดูล SIM800L ในตัวบอร์ด TTGO T-Call V1.4
- ส่งข้อมูลผ่าน HTTP POST ไปยังเซิร์ฟเวอร์
- เหมาะสำหรับการใช้งานในพื้นที่ห่างไกลที่ไม่มี Wi-Fi

### โหมด Wi-Fi
- เชื่อมต่อกับ Wi-Fi hotspot
- ส่งข้อมูลผ่าน HTTP POST ไปยังเซิร์ฟเวอร์เดียวกัน
- เหมาะสำหรับการใช้งานในพื้นที่ที่มี Wi-Fi

การเลือกโหมดเครือข่ายสามารถทำได้ผ่านเมนูหรือตั้งค่าในโค้ด

## ฮาร์ดแวร์ที่ใช้

- บอร์ด ESP32 (TTGO T-Call V1.4)
- จอ LCD I2C 20x4
- Rotary encoder
- ADS1115 ADC module
- DS18B20 temperature sensor
- GPS module (TinyGPSPlus)
- SIM800L/GPRS module (TinyGSM)
- Buzzer และ RGB LED

## การติดตั้งและคอมไพล์

1. เปิดโปรเจกต์ด้วย PlatformIO
2. ตรวจสอบ `platformio.ini`
3. กด build หรือ upload เพื่อแฟลชโค้ดไปยังบอร์ด

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps =
  marcoschwartz/LiquidCrystal_I2C@^1.1.4
  madhephaestus/ESP32Encoder@^0.12.0
  adafruit/Adafruit ADS1X15@^2.6.2
  paulstoffregen/OneWire@^2.3.8
  milesburton/DallasTemperature@^4.0.6
  mikalhart/TinyGPSPlus@^1.1.0
  vshymanskyy/TinyGSM@^0.12.0
  arduino-libraries/ArduinoHttpClient@^0.6.1
  bblanchon/ArduinoJson@^7.2.2

board_build.flash_mode = dio
monitor_speed = 115200
```

## ค่าการตั้งค่าในโค้ด

ดูไฟล์ `src/Config.h` สำหรับการตั้งค่าต่อไปนี้:

- `ENC_CLK`, `ENC_DT`, `ENC_SW` - พิน rotary encoder
- `RGB_R`, `RGB_G`, `RGB_B` - พิน LED
- `BUZZER` - พิน buzzer
- `ONE_WIRE` - พิน DS18B20
- `GPS_RX`, `GPS_TX` - พิน GPS
- `WIFI_SSID`, `WIFI_PASS` - ชื่อและรหัสผ่าน Wi-Fi hotspot
- `SIM_APN` - APN ของซิม
- `HTTP_HOST`, `HTTP_PORT`, `HTTP_PATH` - endpoint สำหรับส่งข้อมูล
- `DEVICE_ID` - รหัสอุปกรณ์

## โครงสร้างหลักของโค้ด

- `src/main.cpp` - สร้าง task และตั้งค่าเริ่มต้น
- `src/ScreenManager.cpp` - แสดงผลบน LCD
- `src/StateMachine.cpp` - ควบคุมสถานะและเมนู
- `src/Sensor.cpp` - อ่านค่าจาก ADS1115 และ DS18B20
- `src/Sim.cpp` - จัดการการเชื่อมต่อ SIM/GPRS และส่ง HTTP
- `src/WifiTask.cpp` - จัดการการเชื่อมต่อ Wi-Fi และส่ง HTTP
- `src/SalinityCalc.cpp` - คำนวณความเค็มและ calibration
- `src/SoundManager.cpp` - ควบคุมเสียง buzzer
- `src/Alarmtask.cpp` - แจ้งเตือน LED/Buzzer ตามค่าความเค็ม

## หมายเหตุ

- ตรวจสอบว่า SIM ได้เปิดใช้งานและใส่ซิมเรียบร้อย
- สำหรับโหมด Wi-Fi ให้ตั้งค่า `WIFI_SSID` และ `WIFI_PASS` ให้ตรงกับ hotspot ที่ต้องการเชื่อมต่อ
- สามารถเปลี่ยนโหมดเครือข่ายระหว่าง SIM และ Wi-Fi ได้ตามความเหมาะสมของสภาพแวดล้อม
- ปรับ `SIM_APN` ให้ตรงกับผู้ให้บริการ (DTAC/True/AIS)
- แหล่งจ่ายไฟต้องเพียงพอสำหรับ SIM800L
- หน้าจอ `System Info` จะแสดง Device ID และ Sensor CH เป็น `CH0`

## ผู้พัฒนา

- AIOT LAB
