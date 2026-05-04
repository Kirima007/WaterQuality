#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "config.h"
#include "shared.h" // ไฟล์ที่มี Struct ของ SensorData, GPSData

#define WIFI_TASK_STACK     8192
#define WIFI_TASK_PRIORITY  1
#define WIFI_TASK_CORE      0

class StateMachine; // Forward declaration

class WifiTask {
public:
    static void taskEntry(void* param);

    // เช็คสถานะ
    static bool isConnected();
    static int  getSignalQuality(); // RSSI

    // สั่งให้ส่งข้อมูล (เรียกจาก StateMachine)
    static void requestSend();
    static void requestSendCalib();

private:
    static volatile bool _connected;
    static volatile bool _sendRequested;
    static volatile bool _sendCalibRequested;
    static volatile int  _signalQuality;

    static String _buildJson(const SensorData& sensor, const GPSData& gps);
    static String _buildCalibJson();
    static bool   _doPost(const String& path, const String& payload, StateMachine* sm);
};