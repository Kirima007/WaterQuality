#pragma once

// --- การตั้งค่า Pins ---
#define ENC_CLK 32
#define ENC_DT 33
#define ENC_SW 14
#define BUZZER 2
#define ONE_WIRE 18
#define GPS_RX 19
#define GPS_TX 25


#if SENSOR_COUNT == 1
    #define RGB_R 9
    #define RGB_G 13
    #define RGB_B 12
#else
    #define RGB_R 15
    #define RGB_G 13
    #define RGB_B 12
#endif

#define STARTUP_DELAY_MS 1500   // หน้า star+tup แสดงกี่ ms

#define SIM_APN         "internet"        // APN ของซิม (DTAC=internet, True=true, AIS=ais)
#define HTTP_HOST       "161.246.157.210" // โฮสต์ปลายทาง
#define HTTP_PORT       80                // พอร์ต
#define HTTP_PATH       "/api/data"     // endpoint
#define HTTP_PATH_CALIB "/api/calibrate/salinity"
#define DEVICE_ID       4           
#define FW_VERSION  "V1.1"     // Version ของเฟิร์มแวร์


#define WIFI_SSID "BCK-WIFI"    // ชื่อ Hotspot ที่ให้ผู้ใช้ตั้งตาม
#define WIFI_PASS "123456789"        // รหัสผ่าน Hotspot


#define NET_MODE_SIM  0             // โหมด 0 คือใช้ SIM800L
#define NET_MODE_WIFI 1             // โหมด 1 คือใช้ Wi-Fi