#pragma once

// --- การตั้งค่า Pins ---
#define ENC_CLK 32
#define ENC_DT 33
#define ENC_SW 14
#define RGB_R 9
#define RGB_G 13
#define RGB_B 12
#define BUZZER 2
#define ONE_WIRE 18
#define GPS_RX 19
#define GPS_TX 25

#define STARTUP_DELAY_MS 1500   // หน้า startup แสดงกี่ ms

#define SIM_APN         "internet"        // APN ของซิม (DTAC=internet, True=true, AIS=ais)
#define HTTP_HOST       "161.246.157.210" // โฮสต์ปลายทาง
#define HTTP_PORT       80                // พอร์ต
#define HTTP_PATH       "/api/data"     // endpoint
#define HTTP_PATH_CALIB "/api/calibrate/salinity"
#define DEVICE_ID       99                 // ID ของเครื่องนี้
#define SIM_PUBLISH_MS  30000             // ส่งทุก 30 วิ