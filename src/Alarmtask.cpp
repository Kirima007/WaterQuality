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
    pinMode(BUZZER, OUTPUT);
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
        if (xQueuePeek(sensorQueue, &sensor, 0) == pdTRUE) {
            
            float ppt   = sensor.currentPPT;
            float thG   = NVSManager::thresh.green;
            float thY   = NVSManager::thresh.yellow;
            float thR   = NVSManager::thresh.red;

            // 2. ปรับลอจิกการเตือนให้ครบทุกช่วง
            if (ppt <= thG) {
                // โซนปลอดภัย → เขียว
                self->_setRGB(0, 1, 0);

            } else if (ppt <= thY) {
                // โซนเฝ้าระวัง → เหลือง + เตือนห่างๆ
                self->_setRGB(1, 1, 0);
                alarmStep = (alarmStep + 1) % 8;
                if (alarmStep == 0) {
                    if (!NVSManager::config.isMuted) tone(BUZZER, 3500, 100);
                }

            } else if (ppt <= thR) {
                // โซนอันตรายระดับ 1 → แดง + เตือนถี่
                self->_setRGB(1, 0, 0);
                alarmStep = (alarmStep + 1) % 8;
                if (alarmStep == 0 || alarmStep == 2 || alarmStep == 4) {
                    if (!NVSManager::config.isMuted) tone(BUZZER, 3500, 100);
                }

            } else {
                alarmStep = (alarmStep + 1) % 8;
                self->_setRGB(alarmStep % 2, 0, 0); // ไฟแดงกระพริบ
                if (!NVSManager::config.isMuted) tone(BUZZER, 3500, 200);            // เสียงเตือนยาวขึ้น
            }

        }
        else {
            self->_allOff();
        }

        vTaskDelay(pdMS_TO_TICKS(ALARM_TASK_DELAY_MS));
    }
}