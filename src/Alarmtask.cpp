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
        return s == AppState::MAIN_SCREEN;
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
            
#if SENSOR_COUNT == 1
            float val   = sensor.valPPT;
            float thG   = NVSManager::thresh.green;
            float thY   = NVSManager::thresh.yellow;
            float thR   = NVSManager::thresh.red;

            // ตรวจสอบค่า Salinity (PPT)
            if (val <= thG) {
                self->_setRGB(0, 1, 0); // เขียว
            } else if (val <= thY) {
                self->_setRGB(1, 1, 0); // เหลือง
                alarmStep = (alarmStep + 1) % 8;
                if (alarmStep == 0 && !NVSManager::config.isMuted) tone(BUZZER, 3500, 100);
            } else if (val <= thR) {
                self->_setRGB(1, 0, 0); // แดง
                alarmStep = (alarmStep + 1) % 8;
                if ((alarmStep == 0 || alarmStep == 2 || alarmStep == 4) && !NVSManager::config.isMuted) tone(BUZZER, 3500, 100);
            } else {
                alarmStep = (alarmStep + 1) % 8;
                self->_setRGB(alarmStep % 2, 0, 0); // แดงกระพริบ
                if (!NVSManager::config.isMuted) tone(BUZZER, 3500, 200);
            }
#else
            // --- 3 Channels Logic ---
            int level = 0; 
            uint8_t currentParam = self->_sm.currentParam;

            if (currentParam == 0) { // Salinity
                if (sensor.valPPT > NVSManager::threshEC.red + 5.0f) level = 3;
                else if (sensor.valPPT > NVSManager::threshEC.red) level = 2;
                else if (sensor.valPPT > NVSManager::threshEC.yellow) level = 1;
                else level = 0;
            } else if (currentParam == 1) { // pH
                if (sensor.valPH > NVSManager::threshPH.red + 1.0f || sensor.valPH < 3.0f) level = 3;
                else if (sensor.valPH > NVSManager::threshPH.red || sensor.valPH < 4.0f) level = 2;
                else if (sensor.valPH > NVSManager::threshPH.yellow || sensor.valPH < 5.0f) level = 1;
                else level = 0;
            } else if (currentParam == 2) { // DO
                if (sensor.valDO < NVSManager::threshDO.red - 1.0f) level = 3;
                else if (sensor.valDO < NVSManager::threshDO.red) level = 2;
                else if (sensor.valDO < NVSManager::threshDO.yellow) level = 1;
                else level = 0;
            }

            // แสดงผลตามระดับของเซ็นเซอร์ที่กำลังดูอยู่
            if (level == 0) {
                self->_setRGB(0, 1, 0); // เขียว
            } else if (level == 1) {
                self->_setRGB(1, 1, 0); // เหลือง
                alarmStep = (alarmStep + 1) % 8;
                if (alarmStep == 0 && !NVSManager::config.isMuted) tone(BUZZER, 3500, 100);
            } else if (level == 2) {
                self->_setRGB(1, 0, 0); // แดง
                alarmStep = (alarmStep + 1) % 8;
                if ((alarmStep == 0 || alarmStep == 2 || alarmStep == 4) && !NVSManager::config.isMuted) tone(BUZZER, 3500, 100);
            } else {
                alarmStep = (alarmStep + 1) % 8;
                self->_setRGB(alarmStep % 2, 0, 0); // แดงกระพริบ
                if (!NVSManager::config.isMuted) tone(BUZZER, 3500, 200);
            }
#endif
        }
        else {
            self->_allOff();
        }

        vTaskDelay(pdMS_TO_TICKS(ALARM_TASK_DELAY_MS));
    }
}