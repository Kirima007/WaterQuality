#include "Rotary.h"

// ==========================================
// Constructor & Initialization
// ==========================================
RotaryInput::RotaryInput(uint8_t pinCLK, uint8_t pinDT, uint8_t pinSW) {
    _pinCLK = pinCLK;
    _pinDT  = pinDT;
    _pinSW  = pinSW;

    _lastEncoderCount  = 0;
    _isButtonDown      = false;
    _longPressTriggered = false;
    _buttonDownTime    = 0;
}

void RotaryInput::begin() {
    // ตั้งค่า Encoder
    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    _encoder.attachHalfQuad(_pinDT, _pinCLK);
    _encoder.setCount(0);

    // ตั้งค่าปุ่ม
    pinMode(_pinSW, INPUT_PULLUP);
}

// ==========================================
// FreeRTOS Task Entry Point
// ==========================================
void RotaryInput::taskEntry(void* param) {
    RotaryInput* self = static_cast<RotaryInput*>(param);

    for (;;) {
        long currentCount = self->_encoder.getCount();

        self->_handleRotation(currentCount);
        self->_handleButton();

        self->_lastEncoderCount = currentCount;

        vTaskDelay(pdMS_TO_TICKS(ROTARY_TASK_DELAY_MS));
    }
}

// ==========================================
// Private: จัดการการหมุน Encoder
// ==========================================
void RotaryInput::_handleRotation(long currentCount) {
    // ถ้าไม่มีการหมุน ให้จบการทำงาน
    if (currentCount == _lastEncoderCount) return; 

    // ✅ ดักจับเฉพาะ "เลขคู่" (จังหวะขอบขาลง / บิดออกจากล็อค)
    if (currentCount % 2 == 0) {
        ButtonEvent ev;

        if (currentCount > _lastEncoderCount) {
            ev = ButtonEvent::ROTATE_CW;   // หมุนขวา
        } else {
            ev = ButtonEvent::ROTATE_CCW;  // หมุนซ้าย
        }

        // ส่ง event เข้า Queue และเล่นเสียง
        xQueueSend(inputQueue, &ev, 0);
    }
    
    // หมายเหตุ: จังหวะเลขคู่ (เข้าล็อค) โค้ดจะไม่ส่ง Event ทำให้ไม่เกิดอาการเบิ้ล
}

// ==========================================
// Private: จัดการการกดปุ่ม (Short / Long Press)
// ==========================================
void RotaryInput::_handleButton() {
    bool isPressed = (digitalRead(_pinSW) == LOW);

    // --- จังหวะกดลง ---
    if (isPressed && !_isButtonDown) {
        _isButtonDown       = true;
        _longPressTriggered = false;
        _buttonDownTime     = millis();
    }

    // --- ระหว่างกดค้าง: เช็ค Long Press ---
    if (isPressed && _isButtonDown && !_longPressTriggered) {
        if (millis() - _buttonDownTime > ROTARY_LONG_PRESS_MS) {
            _longPressTriggered = true;

            ButtonEvent ev = ButtonEvent::LONG_PRESS;
            xQueueSend(inputQueue, &ev, 0);
        }
    }

    // --- จังหวะปล่อยปุ่ม ---
    if (!isPressed && _isButtonDown) {
        _isButtonDown = false;

        unsigned long pressDuration = millis() - _buttonDownTime;

        // Short Press: ปล่อยก่อน Long Press threshold
        if (!_longPressTriggered && pressDuration > ROTARY_DEBOUNCE_MS) {
            ButtonEvent ev = ButtonEvent::SHORT_PRESS;
            xQueueSend(inputQueue, &ev, 0);
        }
    }
}