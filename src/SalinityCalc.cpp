#include "SalinityCalc.h"

// ==========================================
// Constants
// ==========================================
const float SalinityCalc::TARGET_PPT = 35.0f;
const float SalinityCalc::TEMP_COEFF = 0.021f;
const float SalinityCalc::REF_TEMP   = 25.0f;

// ==========================================
// Temperature Compensation
// แปลงแรงดันที่อุณหภูมิใดๆ → แรงดันที่ 25°C
// สูตร: V25 = V / (1 + 0.021 * (T - 25))
// ==========================================
float SalinityCalc::compensate25(float volt, float tempC) {
    return volt / (1.0f + TEMP_COEFF * (tempC - REF_TEMP));
}

// ==========================================
// Calculate Salinity
//
// หลักการ:
// 1. ชดเชยอุณหภูมิทั้ง volt และ v_salt ให้เป็นที่ 25°C
// 2. หาสัดส่วนระหว่าง (ค่าที่อ่าน - DI) / (salt - DI)
// 3. คูณด้วย 35 ppt
//
// ถ้าผลลัพธ์ติดลบ → คืน 0 (น้ำจืดกว่า DI water)
// ==========================================
float SalinityCalc::calculate(float volt, float tempC,
                               float v_di, float v_salt, float t_salt) {
    // ชดเชยอุณหภูมิ
    float volt25   = compensate25(volt,   tempC);
    float vsalt25  = compensate25(v_salt, t_salt);

    // ป้องกัน division by zero
    if (vsalt25 - v_di == 0) return 0.0f;

    // คำนวณ ppt
    float ppt = ((volt25 - v_di) / (vsalt25 - v_di)) * TARGET_PPT;

    return max(0.0f, ppt);
}