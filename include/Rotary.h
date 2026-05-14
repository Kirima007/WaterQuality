#pragma once
#include <Arduino.h>
#include <ESP32Encoder.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "shared.h"     // ButtonEvent, inputQueue
#include "SoundManager.h" // requestSound()

// ค่าตั้งต้นที่ปรับได้
#define ROTARY_DEBOUNCE_MS   10    // debounce ปุ่ม
#define ROTARY_LONG_PRESS_MS 1500  // กดค้างกี่ ms ถึงเป็น Long Press
#define ROTARY_SUPER_LONG_PRESS_MS 10000 // กดค้าง 10 วินาที
#define ROTARY_TASK_DELAY_MS 20    // ความเร็วในการสแกน

class RotaryInput {
public:
    // Constructor รับ pin ทั้ง 3 ตัว
    RotaryInput(uint8_t pinCLK, uint8_t pinDT, uint8_t pinSW);

    // เรียกใน setup() ก่อนสร้าง Task
    void begin();

    // FreeRTOS Task Entry Point
    static void taskEntry(void* param);

private:
    // Pins
    uint8_t _pinCLK;
    uint8_t _pinDT;
    uint8_t _pinSW;

    // Encoder object
    ESP32Encoder _encoder;

    // สถานะปุ่ม
    long          _lastEncoderCount;
    bool          _isButtonDown;
    bool          _longPressTriggered;
    bool          _superLongPressTriggered;
    unsigned long _buttonDownTime;

    // Internal handlers
    void _handleRotation(long currentCount);
    void _handleButton();
};