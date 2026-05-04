#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "shared.h"
#include "NVSManager.h"
#include "Config.h"

// ==========================================
// ตั้งค่า Server
// ==========================================


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
    static bool isConnected();  // SIM/GPRS ต่ออยู่ไหม
    static int  getSignalQuality();
    static void requestSend(); // เรียกจาก StateMachine เมื่ออยากส่งข้อมูล
    static void requestSendCalib(); // เรียกจาก StateMachine เมื่ออยากส่งข้อมูลการปรับค่า


private:
    static volatile bool _connected;
    static volatile bool _sendRequested;
    static volatile bool _sendCalibRequested;
    static volatile int  _signalQuality;
    
    

    // สร้าง JSON payload
    static String _buildJson(const SensorData& sensor, const GPSData& gps);
    static String _buildCalibJson();
};