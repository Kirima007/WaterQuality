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
    _superLongPressTriggered = false;
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

        // ปล่อยให้ _handleRotation ควบคุม _lastEncoderCount เอง เพื่อไม่ให้เสียเศษทิ้งเวลาหมุนไวๆ

        vTaskDelay(pdMS_TO_TICKS(ROTARY_TASK_DELAY_MS));
    }
}

// ==========================================
// Private: จัดการการหมุน Encoder
// ==========================================
void RotaryInput::_handleRotation(long currentCount) {
    long diff = currentCount - _lastEncoderCount;

    // 1 ล็อคของ Encoder แบบ Half Quad คือ 2 count
    if (abs(diff) >= 2) {
        int clicks = diff / 2; // คำนวณจำนวนคลิกที่หมุนได้จริงในช่วง 20ms
        ButtonEvent ev = (clicks > 0) ? ButtonEvent::ROTATE_CW : ButtonEvent::ROTATE_CCW;
        
        int eventsToSend = abs(clicks);
        for (int i = 0; i < eventsToSend; i++) {
            xQueueSend(inputQueue, &ev, 0); // โยน event เข้าระบบตามจำนวนที่หมุนจริง
        }
        
        // อัปเดตค่า _lastEncoderCount เฉพาะส่วนที่นับเป็นคลิกแล้ว (เพื่อเก็บเศษติ่งไว้คำนวณรอบถัดไป)
        _lastEncoderCount += (clicks * 2);
    }
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
        _superLongPressTriggered = false;
        _buttonDownTime     = millis();
    }

    // --- ระหว่างกดค้าง: เช็ค Long Press & Super Long Press ---
    if (isPressed && _isButtonDown) {
        unsigned long pressTime = millis() - _buttonDownTime;
        
        if (!_superLongPressTriggered && pressTime > ROTARY_SUPER_LONG_PRESS_MS) {
            _superLongPressTriggered = true;
            ButtonEvent ev = ButtonEvent::SUPER_LONG_PRESS;
            xQueueSend(inputQueue, &ev, 0);
        }
        else if (!_longPressTriggered && pressTime > ROTARY_LONG_PRESS_MS) {
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