#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

// Sound events ที่ Task อื่นส่งมาขอเล่นเสียง
enum class SoundEvent {
    SCROLL,
    SELECT,
    BACK,
    SUCCESS
};

class SoundManager {
public:
    // Constructor & Initialization
    SoundManager(uint8_t pin);
    void begin();

    // Mute Control
    void setMute(bool muteState);
    bool isMuted();

    // Sound Patterns (เรียกตรงจาก SoundTask เท่านั้น)
    void beepScroll();
    void beepSelect();
    void beepBack();
    void beepSuccess();

    // FreeRTOS Task Entry Point
    static void taskEntry(void* param);

private:
    uint8_t _buzzerPin;
    bool    _isMuted;
};

// Queue สำหรับรับ SoundEvent จาก Task อื่น
// ประกาศที่นี่ สร้างจริงใน SoundManager.cpp
extern QueueHandle_t soundQueue;

// Helper function สำหรับ Task อื่นส่งเสียงได้สะดวก
inline void requestSound(SoundEvent ev) {
    if (soundQueue != nullptr) {
        xQueueSend(soundQueue, &ev, 0);
    }
}