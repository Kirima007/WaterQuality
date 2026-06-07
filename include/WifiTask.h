#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "config.h"
#include "shared.h" // ไฟล์ที่มี Struct ของ SensorData, GPSData


class StateMachine; // Forward declaration

class WifiTask {
public:
    static void taskEntry(void* param);

    // เช็คสถานะ
    static bool isConnected();
    static int  getSignalQuality(); // RSSI

    // สั่งให้ส่งข้อมูล (เรียกจาก StateMachine)
    static void requestSend(uint8_t sensorIndex = 0);
    static void requestSendCalib(uint8_t sensorIndex = 0);
    static void requestCheckOta();
    static void requestStartOta(const String& url);

private:
    static volatile bool _connected;
    static volatile bool _sendRequested;
    static volatile bool _sendCalibRequested;
    static volatile bool _checkOtaRequested;
    static volatile bool _startOtaRequested;
    static String _otaUrl;
    static volatile uint8_t _reqSensorIdx;
    static volatile uint8_t _reqCalibIdx;
    static volatile int  _signalQuality;

    static String _buildJson(const SensorData& sensor, const GPSData& gps);
    static String _buildCalibJson();
    static bool   _doPost(const String& path, const String& payload, StateMachine* sm);
};