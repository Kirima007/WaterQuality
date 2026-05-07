#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

struct SensorData {
    float currentTemp; // อุณหภูมิ °C
    float currentVolt; // แรงดันจาก ADS1115
    float currentPPT; // PPT
    float currentEC;
    long int timestamp; 
};

struct GPSData {
    double lat, lng; //ละติจูดและลองจิจูด
    bool valid; //GPS lock แล้วหรือยัง
    uint8_t satellites; //จำนวนดาวเทียมที่จับได้
};

enum class ButtonEvent {
    SHORT_PRESS,
    LONG_PRESS,
    ROTATE_CW,
    ROTATE_CCW
};

// Queue handles (ประกาศใน main.ino, extern ที่นี่)
extern QueueHandle_t sensorQueue;
extern QueueHandle_t gpsQueue;
extern QueueHandle_t inputQueue;