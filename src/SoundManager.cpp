#include "SoundManager.h"

// สร้าง Queue จริงที่นี่ ไฟล์อื่น extern มาใช้
QueueHandle_t soundQueue = nullptr;

// --- Constructor & Initialization ---
SoundManager::SoundManager(uint8_t pin) {
    _buzzerPin = pin;
    _isMuted   = false;
}

void SoundManager::begin() {
    pinMode(_buzzerPin, OUTPUT);
    digitalWrite(_buzzerPin, LOW); // ป้องกันเสียงค้างตอนเริ่มระบบ

    // สร้าง Queue จุได้ 5 event
    // ถ้าเล่นเสียงไม่ทัน event ใหม่จะรอในคิวก่อน
    soundQueue = xQueueCreate(5, sizeof(SoundEvent));
}

// --- Mute Control ---
void SoundManager::setMute(bool muteState) {
    _isMuted = muteState;
}

bool SoundManager::isMuted() {
    return _isMuted;
}

// ==========================================
// Sound Patterns
// เรียกจาก SoundTask เท่านั้น ไม่เรียกตรงจาก Task อื่น
// เพราะมี vTaskDelay ที่จะบล็อค Task ที่เรียก
// ==========================================
void SoundManager::beepScroll() {
    if (_isMuted) return;
    tone(_buzzerPin, 3000, 15);
    // beepScroll สั้นมาก ไม่ต้อง delay รอ
}

void SoundManager::beepSelect() {
    if (_isMuted) return;
    tone(_buzzerPin, 2000, 50);
    vTaskDelay(pdMS_TO_TICKS(60));
    tone(_buzzerPin, 2500, 100);
}

void SoundManager::beepBack() {
    if (_isMuted) return;
    tone(_buzzerPin, 2500, 50);
    vTaskDelay(pdMS_TO_TICKS(60));
    tone(_buzzerPin, 2000, 100);
}

void SoundManager::beepSuccess() {
    if (_isMuted) return;
    tone(_buzzerPin, 2000, 100);
    vTaskDelay(pdMS_TO_TICKS(120));
    tone(_buzzerPin, 2500, 100);
    vTaskDelay(pdMS_TO_TICKS(120));
    tone(_buzzerPin, 3000, 200);
}

// ==========================================
// FreeRTOS Task
// วนรับ SoundEvent จาก Queue แล้วเล่นเสียง
// ==========================================
void SoundManager::taskEntry(void* param) {
    SoundManager* self = static_cast<SoundManager*>(param);
    SoundEvent ev;

    for (;;) {
        // รอจนมี event เข้ามา ถ้าไม่มีก็นอนรอ ไม่กิน CPU
        if (xQueueReceive(soundQueue, &ev, portMAX_DELAY) == pdTRUE) {
            switch (ev) {
                case SoundEvent::SCROLL:  self->beepScroll();  break;
                case SoundEvent::SELECT:  self->beepSelect();  break;
                case SoundEvent::BACK:    self->beepBack();    break;
                case SoundEvent::SUCCESS: self->beepSuccess(); break;
            }
        }
    }
}