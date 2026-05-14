#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "shared.h"

#define GPS_TASK_DELAY_MS  500

class GPSTask {
public:
    static void taskEntry(void* param);
};