#pragma once
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "shared.h"
#include "StateMachine.h"
#include "NVSManager.h"

#define DISPLAY_TASK_DELAY_MS  25    // วาด LCD ทุก 25ms (40fps)

class ScreenManager {
public:
    ScreenManager(LiquidCrystal_I2C& lcd, StateMachine& sm);

    void begin();

    // FreeRTOS Task Entry Point
    static void taskEntry(void* param);

private:
    LiquidCrystal_I2C& _lcd;
    StateMachine&      _sm;

    // ข้อมูลล่าสุดจาก Queue
    SensorData _sensor{};
    GPSData    _gps{};

    // ดึงข้อมูลจาก Queue
    void _updateData();

    // วาดแต่ละหน้า (ใช้ร่วมกันทั้ง 1 หัวและ 3 หัว)
    void _drawStartup();
    void _drawMainScreen();
    void _drawMainMenu();
    void _drawReadTemp();
    void _drawReadGPS();
    void _drawThreshMenu();
    void _drawEditThresh();
    void _drawCalMenu();
    void _drawTempCal();
    void _drawSystemInfo();
    void _drawCalmanual();
    void _drawEditCalManual();
    void _drawCal1();
    void _drawCal2();
    void _drawCalFinish(); 
    void _drawCalCancelConfirm();
    void _drawSimSending(); 
    void _drawSimResult();
    void _drawNetworkStatus();
    void _drawSystemSetup();

#if SENSOR_COUNT == 3
    // วาดหน้าจอที่มีเฉพาะในรุ่น 3 หัว
    void _drawSensorSelectMenu();
#endif

    // Helper: พิมพ์ตัวเลขให้ชิดขวาในความกว้างที่กำหนด
    void _printPadded(float val, int decimals, int width);
};