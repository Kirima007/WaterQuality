#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "shared.h"
#include "NVSManager.h"
#include "StateMachine.h"
#include "config.h"

#define ALARM_TASK_DELAY_MS  300

class AlarmTask {
public:
    AlarmTask(const StateMachine& sm);

    static void taskEntry(void* param);

private:
    const StateMachine& _sm;

    void _setRGB(bool r, bool g, bool b);
    void _allOff();
};