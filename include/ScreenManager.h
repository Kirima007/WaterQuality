#pragma once
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "shared.h"
#include "StateMachine.h"
#include "NVSManager.h"

#define DISPLAY_TASK_DELAY_MS  25    // วาด LCD ทุก 25ms (40fps)
#define DISPLAY_TASK_STACK     8192
#define DISPLAY_TASK_PRIORITY  1
#define DISPLAY_TASK_CORE      1

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

    // วาดแต่ละหน้า
    void _drawStartup();
    void _drawMainScreen();
    void _drawMainMenu();
    void _drawReadTemp();
    void _drawReadGPS();
    void _drawThreshMenu();
    void _drawEditThresh();
    void _drawCalMenu(); // new
    void _drawCalmanual(); // new
    void _drawEditCalManual(); // new
    void _drawCalDI();
    void _drawCalSalt();
    void _drawCalFinish(); 

    
    void _drawCalCancelConfirm();
    void _drawSimStatus();
    void _drawSimSending(); 
    void _drawSimResult();

    // Helper: พิมพ์ตัวเลขให้ชิดขวาในความกว้างที่กำหนด
    void _printPadded(float val, int decimals, int width);
};