#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include "config.h"
#include "shared.h"
#include "NVSManager.h"
#include "SoundManager.h"
#include "Rotary.h"
#include "SensorTask.h"
#include "GPS.h"
#include "StateMachine.h"
#include "ScreenManager.h"
#include "AlarmTask.h"
#include "SimTask.h"
#include "WifiTask.h"

// ==========================================
// Objects
// ==========================================
LiquidCrystal_I2C lcd(0x27, 20, 4);
SoundManager      soundMgr(BUZZER);
RotaryInput       rotary(ENC_CLK, ENC_DT, ENC_SW);
StateMachine      stateMachine;
ScreenManager     screenMgr(lcd, stateMachine);
AlarmTask         alarmTask(stateMachine);
// ==========================================
// Queue Handles (สร้างจริงที่นี่)
// ==========================================
QueueHandle_t sensorQueue = nullptr;
QueueHandle_t gpsQueue    = nullptr;
QueueHandle_t inputQueue  = nullptr;

// ==========================================
// DisplayTask — รับ input แล้วส่งให้ StateMachine
// (รันบน Core 1 คู่กับ ScreenManager)
// ==========================================
void displayTask(void* param) {
    StateMachine* sm = static_cast<StateMachine*>(param);
    ButtonEvent ev;

    requestSound(SoundEvent::SELECT);
    vTaskDelay(pdMS_TO_TICKS(STARTUP_DELAY_MS));
    sm->begin();

    for (;;) {
        if (xQueueReceive(inputQueue, &ev, portMAX_DELAY) == pdTRUE) {
            sm->handleEvent(ev);
        }
    }
}

// ==========================================
// SETUP
// ==========================================
void setup() {
    Serial2.begin(115200);
    Serial2.println("Hello");
    // --- I2C ---
    Wire.begin(21, 22);

    // --- โหลดค่า Calibration + Threshold จาก Flash ---
    NVSManager::load();

    // --- สร้าง Queue ทั้งหมดก่อนสร้าง Task ---
    sensorQueue = xQueueCreate(1,  sizeof(SensorData));
    gpsQueue    = xQueueCreate(1,  sizeof(GPSData));
    inputQueue  = xQueueCreate(10, sizeof(ButtonEvent));

    // --- Init Hardware ---
    soundMgr.begin();   // สร้าง soundQueue ด้วยในตัว
    rotary.begin();
    screenMgr.begin();

    // ==========================================
    // สร้าง Tasks
    // ==========================================

    // --- Core 0: Sensing ---
    xTaskCreatePinnedToCore(
        SensorTask::taskEntry, "Sensor",
        SENSOR_TASK_STACK, nullptr,
        SENSOR_TASK_PRIORITY, nullptr, SENSOR_TASK_CORE
    );

    xTaskCreatePinnedToCore(
        GPSTask::taskEntry, "GPS",
        GPS_TASK_STACK, nullptr,
        GPS_TASK_PRIORITY, nullptr, GPS_TASK_CORE
    );

    xTaskCreatePinnedToCore(
        SoundManager::taskEntry, "Sound",
        2048, &soundMgr,
        1, nullptr, 0
    );

    xTaskCreatePinnedToCore(
        AlarmTask::taskEntry, "Alarm",
        ALARM_TASK_STACK, &alarmTask,
        ALARM_TASK_PRIORITY, nullptr, ALARM_TASK_CORE
    );
    xTaskCreatePinnedToCore(
        SimTask::taskEntry, "SIM",
        SIM_TASK_STACK, &stateMachine,
        SIM_TASK_PRIORITY, nullptr, SIM_TASK_CORE
    );
    xTaskCreatePinnedToCore(
        WifiTask::taskEntry, "WiFi",
        WIFI_TASK_STACK, &stateMachine,
        WIFI_TASK_PRIORITY, nullptr, WIFI_TASK_CORE
        );


    // --- Core 1: UI ---
    xTaskCreatePinnedToCore(
        ScreenManager::taskEntry, "Screen",
        DISPLAY_TASK_STACK, &screenMgr,
        DISPLAY_TASK_PRIORITY, nullptr, DISPLAY_TASK_CORE
    );

    xTaskCreatePinnedToCore(
        displayTask, "Input",
        4096, &stateMachine,
        3, nullptr, 1    // priority สูง ตอบสนองเร็ว
    );

    xTaskCreatePinnedToCore(
        RotaryInput::taskEntry, "Rotary",
        2048, &rotary,            // ส่ง object rotary เข้าไปทำงาน
        2, nullptr, 1             // Priority กลางๆ รันบน Core 1
    );

}

void loop() {
    vTaskDelay(portMAX_DELAY);
}



//made by Weerapat C. 2026