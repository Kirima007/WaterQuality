#pragma once
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "shared.h"
#include "StateMachine.h"
#include "NVSManager.h"

#define DISPLAY_TASK_DELAY_MS  25    // วาด LCD ทุก 25ms (40fps)
#define LCD_I2C_ADDR           0x27  // Address ของจอ LCD (PCF8574)
#define LCD_RETRY_MS           3000  // ถ้าไม่เจอจอ ลองหาใหม่ทุกกี่ ms

class ScreenManager {
public:
    ScreenManager(LiquidCrystal_I2C& lcd, StateMachine& sm);

    void begin();

    // FreeRTOS Task Entry Point
    static void taskEntry(void* param);

private:
    LiquidCrystal_I2C& _lcd;
    StateMachine&      _sm;

    // สถานะจอ: ถ้าไม่ได้ต่อจอ จะไม่เขียน I2C เลย (กัน log error รัวเป็นพันบรรทัด)
    bool     _lcdOk        = false;
    uint32_t _lcdLastProbe = 0;

    // ส่ง address ไปถามว่ามีจออยู่ไหม (transaction เดียว ใช้เวลาไม่ถึง 1ms)
    bool _probeLcd() {
        Wire.beginTransmission(LCD_I2C_ADDR);
        return Wire.endTransmission() == 0;
    }

    // true = จอพร้อมวาด
    // - จอต่ออยู่: เช็คทุกรอบ (จับได้ทันทีถ้าจอหลุดกลางทาง)
    // - จอไม่ได้ต่อ: ลองหาใหม่ทุก LCD_RETRY_MS, เจอแล้ว init ใหม่อัตโนมัติ
    bool _lcdReady() {
        uint32_t now = millis();
        if (_lcdOk || (now - _lcdLastProbe >= LCD_RETRY_MS)) {
            _lcdLastProbe = now;
            bool found = _probeLcd();
            if (found && !_lcdOk) {
                _lcd.init();
                _lcd.backlight();
                _lcd.clear();
            }
            _lcdOk = found;
        }
        return _lcdOk;
    }

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
    void _drawOtaChecking();
    void _drawOtaResult();
    void _drawOtaUpdating();
    void _drawEditDeviceId();

#if SENSOR_COUNT == 3
    // วาดหน้าจอที่มีเฉพาะในรุ่น 3 หัว
    void _drawSensorSelectMenu();
#endif

    // Helper: พิมพ์ตัวเลขให้ชิดขวาในความกว้างที่กำหนด
    void _printPadded(float val, int decimals, int width);
};