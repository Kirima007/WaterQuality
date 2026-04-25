#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "shared.h"
#include "NVSManager.h"

// ==========================================
// ตั้งค่า Server
// ==========================================
#define SIM_APN         "internet"        // APN ของซิม (DTAC=internet, True=true, AIS=ais)
#define HTTP_HOST       "your-server.com" // โฮสต์ปลายทาง
#define HTTP_PORT       80                // พอร์ต
#define HTTP_PATH       "/api/sensor"     // endpoint
#define DEVICE_ID       1                 // ID ของเครื่องนี้
#define SIM_PUBLISH_MS  30000             // ส่งทุก 30 วิ

// SIM800L Pins (T-Call V1.4 built-in)
#define MODEM_RX        26
#define MODEM_TX        27
#define MODEM_POWER_ON  23
#define MODEM_RST        5
#define MODEM_PWRKEY     4

// Task settings
#define SIM_TASK_STACK     8192
#define SIM_TASK_PRIORITY  1
#define SIM_TASK_CORE      0

class SimTask {
public:
    static void taskEntry(void* param);

private:
    // สร้าง JSON payload จากข้อมูล sensor + GPS
    static String _buildJson(const SensorData& sensor, const GPSData& gps);

    // ส่ง HTTP POST คืนค่า true ถ้าสำเร็จ
    static bool _sendHTTP(const String& payload, String& responseOut);
};