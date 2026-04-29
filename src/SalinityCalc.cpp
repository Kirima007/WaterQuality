#include "SalinityCalc.h"

// ==========================================
// Constants
// ==========================================
const float SalinityCalc::TARGET_PPT = 35.0f;

float SalinityCalc::calcEC25(float volt, float tempC) {
    float ecBase = (19.47f * volt) - 0.008f;
    return ecBase / (1.0f + 0.01702f * (tempC - 25.0f));
}

bool SalinityCalc::computeAlphaBeta(float v_di, float t_di, 
                                    float v_salt, float t_salt, 
                                    float &alphaOut, float &betaOut) {
    float ec25_di   = calcEC25(v_di, t_di);
    float ec25_salt = calcEC25(v_salt, t_salt);

    float target_ec_di   = 0.0f;  
    float target_ec_salt = (TARGET_PPT - 0.1634f) / 0.4803f; 

    // ป้องกัน Error หารด้วยศูนย์
    if (abs(ec25_salt - ec25_di) < 0.0001f) return false;

    // สมการ y = mx + c (หรือ alpha*x + beta)
    alphaOut = (target_ec_salt - target_ec_di) / (ec25_salt - ec25_di);
    betaOut  = target_ec_di - (alphaOut * ec25_di);
    return true;
}

float SalinityCalc::calculate(float volt, float tempC, float alpha, float beta) {
    float ec25_current = calcEC25(volt, tempC);
    
    // เอา Alpha/Beta ที่เซฟไว้ใน EEPROM มาปรับแก้ EC
    float ecFinal = (alpha * ec25_current) + beta;
    
    // แปลงกลับเป็น PPT
    float salinity = (0.4803f * ecFinal) + 0.1634f;
    return max(0.0f, salinity);
}