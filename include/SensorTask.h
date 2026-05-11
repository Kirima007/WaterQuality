#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "shared.h"

// Task settings
#define SENSOR_TASK_DELAY_MS  1000  // อ่านค่าทุก 1 วินาที
#define SENSOR_TASK_STACK     4096
#define SENSOR_TASK_PRIORITY  2
#define SENSOR_TASK_CORE      0

class SensorTask {
public:
    // FreeRTOS Task Entry Point
    static void taskEntry(void* param);
};