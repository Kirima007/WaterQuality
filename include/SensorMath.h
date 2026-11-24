#pragma once
#include <Arduino.h>

class SensorMath {
public:
    static const float TARGET_PPT;

    // EC & Salinity
    static float calculate(float volt, float tempC, float alpha, float beta);
    static float calculateEC(float volt, float tempC, float alpha, float beta);
    
    // pH
    static float calculatePH(float volt, float tempC, float alpha, float beta);
    
    // DO
    static float calculateDO(float volt, float tempC, float alpha, float beta);

    static bool computeAlphaBeta(uint8_t currentParam,
                                float v1, float t1, 
                                float v2, float t2, 
                                float &alphaOut, float &betaOut);

    // ==========================================
    // Stability Tracker (ติดตามความนิ่งของค่าเบื้องหลัง)
    // SensorTask เป็นคนเดียวที่ push ค่า, Task อื่นอ่านอย่างเดียว
    // ==========================================
    static const uint8_t STABLE_WINDOW = 15;   // จำนวนค่าล่าสุดที่ใช้ตัดสิน (~15-18 วินาที)

    struct StabilityInfo {
        uint8_t count;   // จำนวนค่าที่มีในหน้าต่าง (0..STABLE_WINDOW)
        float   range;   // max - min ของค่า base ในหน้าต่าง
        bool    stable;  // ครบหน้าต่าง และ range <= threshold
    };

    // เรียกจาก SensorTask ทุกครั้งที่อ่านค่าใหม่
    static void pushSample(uint8_t param, float volt, float tempC);
    // ล้างหน้าต่าง (เช่น ADS หลุด)
    static void clearStability();
    // อ่านสถานะความนิ่ง (เรียกได้จากทุก Task)
    static StabilityInfo getStability(uint8_t param);
    // เกณฑ์ความนิ่งของแต่ละเซ็นเซอร์ (หน่วยเดียวกับค่า base)
    static float stableThreshold(uint8_t param);

    // ดึงค่าเฉลี่ยแรงดัน/อุณหภูมิ ถ้าค่านิ่งแล้ว (ไม่บล็อก) — false = ยังไม่นิ่ง
    static bool captureStableValue(uint8_t currentParam, float &capturedVolt, float &capturedTemp);

private:
    static float calcEC25(float volt, float tempC);
    static float calcPHBase(float volt, float tempC);
    static float calcDOBase(float volt, float tempC);
};