#pragma once
#include <Arduino.h>

class SalinityCalc {
public:
    static const float TARGET_PPT;

    // ฟังก์ชันหลักสำหรับคำนวณ
    static float calculate(float volt, float tempC, float alpha, float beta);
    static float calculateEC(float volt, float tempC, float alpha, float beta);
    static bool computeAlphaBeta(float v_di, float t_di, 
                                float v_salt, float t_salt, 
                                float &alphaOut, float &betaOut);

private:
    // ฟังก์ชันย่อยสำหรับแปลง Voltage เป็น EC ที่ 25 องศา (ตามสูตรรุ่นพี่)
    static float calcEC25(float volt, float tempC);
};