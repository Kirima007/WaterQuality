#include "SensorMath.h"

// ==========================================
// Constants
// ==========================================
// const float SensorMath::TARGET_PPT = 6.35f;

float SensorMath::calcEC25(float volt, float tempC) {
    float ecBase = (19.47f * volt) - 0.008f;
    return ecBase / (1.0f + 0.01702f * (tempC - 25.0f));
}

bool SensorMath::computeAlphaBeta(float v_di, float t_di, 
                                   float v_salt, float t_salt, 
                                   float &alphaOut, float &betaOut) {
    float ec25_di   = calcEC25(v_di, t_di);
    float ec25_salt = calcEC25(v_salt, t_salt);

    float target_ec_di   = 1.413f;   // สำหรับตอนจุ่มน้ำยา 1413 us/cm
    float target_ec_salt = 12.88f;   // สำหรับตอนจุ่มน้ำยา 12.88 ms/cm

    // ป้องกัน Error หารด้วยศูนย์
    if (abs(ec25_salt - ec25_di) < 0.0001f) return false;

    // สมการ y = mx + c (หรือ alpha*x + beta)
    alphaOut = (target_ec_salt - target_ec_di) / (ec25_salt - ec25_di);
    betaOut  = target_ec_di - (alphaOut * ec25_di);
    return true;
}

float SensorMath::calculate(float volt, float tempC, float alpha, float beta) {
    float ecFinal = calculateEC(volt, tempC, alpha, beta);
    
    if (ecFinal <= 0.0f) {//กรองกรณี EC ติดลบหรือเป็นศูนย์ (ซึ่งไม่สมเหตุสมผล) ให้คืนค่า Salinity เป็น 0.0 ppt แทน
        return 0.0f;
    }
    // แปลงจาก EC (mS/cm) เป็น Salinity (PPT)
    // salinity = (0.4803 \times ecFinal) + 0.1634$$
    float salinity = (0.4803f * ecFinal) + 0.1634f;
    return max(0.0f, salinity);
}

float SensorMath::calculateEC(float volt, float tempC, float alpha, float beta) {
    float ec25_current = calcEC25(volt, tempC);
    float ecFinal = (alpha * ec25_current) + beta;
    return max(0.0f, ecFinal);
}