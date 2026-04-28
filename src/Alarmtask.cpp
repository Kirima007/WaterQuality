#include "AlarmTask.h"
#include "SoundManager.h"

// ==========================================
// Constructor
// ==========================================
AlarmTask::AlarmTask(const StateMachine& sm) : _sm(sm) {}

// ==========================================
// Helper
// ==========================================
void AlarmTask::_setRGB(bool r, bool g, bool b) {
    digitalWrite(RGB_R, r ? HIGH : LOW);
    digitalWrite(RGB_G, g ? HIGH : LOW);
    digitalWrite(RGB_B, b ? HIGH : LOW);
}

void AlarmTask::_allOff() {
    _setRGB(false, false, false);
}

// ==========================================
// FreeRTOS Task Entry Point
// ==========================================
void AlarmTask::taskEntry(void* param) {
    AlarmTask* self = static_cast<AlarmTask*>(param);

    // Init RGB pins
    pinMode(RGB_R, OUTPUT);
    pinMode(RGB_G, OUTPUT);
    pinMode(RGB_B, OUTPUT);
    self->_allOff();

    SensorData sensor{};
    int alarmStep = 0;

    // State ที่ต้องการให้ LED ทำงาน
    auto isMonitorState = [](AppState s) {
        return s == AppState::MAIN_SCREEN
            || s == AppState::READ_TEMP
            || s == AppState::READ_GPS;
    };

    for (;;) {
        AppState state = self->_sm.getState();

        // ถ้าไม่อยู่หน้า monitor → ปิด LED ทั้งหมด
        if (!isMonitorState(state)) {
            self->_allOff();
            vTaskDelay(pdMS_TO_TICKS(ALARM_TASK_DELAY_MS));
            continue;
        }

        // ดึงค่าล่าสุด
        xQueuePeek(sensorQueue, &sensor, 0);

        float ppt   = sensor.currentPPT;
        float thG   = NVSManager::thresh.green;
        float thR   = NVSManager::thresh.red;

        if (ppt < thG) {
            // ปกติ → เขียว
            self->_setRGB(false, true, false);

        } else if (ppt < thR) {
            // เตือน → น้ำเงิน
            self->_setRGB(false, false, true);

        } else {
            // อันตราย → แดง + buzzer
            self->_setRGB(true, false, false);

            // // Buzzer เตือนเป็นจังหวะ (3 ครั้งต่อรอบ)
            // alarmStep = (alarmStep + 1) % 8;
            // if (alarmStep == 0 || alarmStep == 2 || alarmStep == 4) {
            //     tone(BUZZER, 3500, 100);
            // }
        }

        vTaskDelay(pdMS_TO_TICKS(ALARM_TASK_DELAY_MS));
    }
}