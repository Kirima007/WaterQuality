#pragma once
#include <Arduino.h>

class SalinityCalc {
public:
    // คำนวณความเค็ม (ppt) จากแรงดันและอุณหภูมิ
    // volt     = แรงดันที่อ่านได้จาก ADS1115
    // tempC    = อุณหภูมิจาก DS18B20
    // v_di     = แรงดัน DI water ตอน calibrate
    // v_salt   = แรงดัน 35ppt solution ตอน calibrate
    // t_salt   = อุณหภูมิตอน calibrate
    static float calculate(float volt, float tempC,
                           float v_di, float v_salt, float t_salt);

    // แปลงแรงดันดิบให้เป็นแรงดันที่อุณหภูมิ 25°C
    // ใช้ temperature compensation factor 0.021 /°C
    static float compensate25(float volt, float tempC);

private:
    static const float TARGET_PPT;       // 35.0 ppt
    static const float TEMP_COEFF;       // 0.021 /°C
    static const float REF_TEMP;         // 25.0 °C
};