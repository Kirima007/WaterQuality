#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

struct SensorData {
    uint32_t timestamp;
    float tempC;      // อุณหภูมิน้ำ
    bool  tempValid;  // อ่านอุณหภูมิสำเร็จไหม
    bool  adsValid;   // ADS1115 ติดต่อได้ไหม

    // --- เซ็นเซอร์ 1: ความเค็ม ---
    float voltEC;     // ค่าแรงดันดิบ
    float valEC;      // ค่า EC (mS/cm)
    float valPPT;     // ค่าความเค็ม (ppt)

#if SENSOR_COUNT == 3
    // --- เซ็นเซอร์ 2: pH ---
    float voltPH;
    float valPH;      // ค่า pH (0-14)

    // --- เซ็นเซอร์ 3: DO ---
    float voltDO;
    float valDO;      // ค่า DO (mg/L)
#endif
};

struct GPSData {
    double lat, lng; //ละติจูดและลองจิจูด
    bool valid; //GPS lock แล้วหรือยัง
    uint8_t satellites; //จำนวนดาวเทียมที่จับได้
    bool serialError; // เช็คว่ามีการส่งข้อมูลจากโมดูล GPS หรือไม่
};

enum class ButtonEvent {
    SHORT_PRESS,
    LONG_PRESS,
    SUPER_LONG_PRESS,
    ROTATE_CW,
    ROTATE_CCW
};

// Queue handles (ประกาศใน main.ino, extern ที่นี่)
extern QueueHandle_t sensorQueue;
extern QueueHandle_t gpsQueue;
extern QueueHandle_t inputQueue;