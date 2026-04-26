#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "shared.h"
#include "NVSManager.h"
#include "StateMachine.h"
#include "config.h"

#define ALARM_TASK_DELAY_MS  300
#define ALARM_TASK_STACK     2048
#define ALARM_TASK_PRIORITY  2
#define ALARM_TASK_CORE      0

class AlarmTask {
public:
    // ต้องการ StateMachine เพื่อเช็คว่าอยู่หน้าไหน
    // LED จะทำงานเฉพาะหน้า MAIN_SCREEN, READ_TEMP, READ_GPS
    AlarmTask(const StateMachine& sm);

    static void taskEntry(void* param);

private:
    const StateMachine& _sm;

    void _setRGB(bool r, bool g, bool b);
    void _allOff();
};