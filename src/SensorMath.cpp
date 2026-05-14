#include "SensorMath.h"
#include "Shared.h"

// ==========================================
// Constants
// ==========================================
// const float SensorMath::TARGET_PPT = 6.35f;

float SensorMath::calcEC25(float volt, float tempC) {
    float ecBase = (19.47f * volt) - 0.008f;
    return ecBase / (1.0f + 0.01702f * (tempC - 25.0f));
}

float SensorMath::calcPHBase(float volt, float tempC) {
    float v_ph7 = 1.62f;   // แรงดันที่ pH 7.00
    float v_ph4 = 2.195f;  // แรงดันที่ pH 4.00
    float v_step_25 = (v_ph4 - v_ph7) / 3.0f; 
    float v_step_T = v_step_25 * ((tempC + 273.15f) / 298.15f); 
    return 7.0f - ((volt - v_ph7) / v_step_T);
}

float SensorMath::calcDOBase(float volt, float tempC) {
    float v0 = -0.032f;   // แรงดันที่ 0% DO
    float v100 = 1.365f; // แรงดันที่ 100% DO
    float percentDO = (volt - v0) / (v100 - v0);
    percentDO = max(0.0f, percentDO);
    float maxDO = 14.46f - (0.38f * tempC) + (0.0054f * tempC * tempC);
    return percentDO * maxDO; 
}

bool SensorMath::computeAlphaBeta(uint8_t currentParam, float v1, float t1, 
                                float v2, float t2, 
                                float &alphaOut, float &betaOut) {
    float m1 = 0, m2 = 0;
    float target1 = 0, target2 = 0;

    if (currentParam == 0) {
        target1 = 1.413f;
        target2 = 12.88f;
        m1 = calcEC25(v1, t1);
        m2 = calcEC25(v2, t2);
    } else if (currentParam == 1) {
        target1 = 4.0f;
        target2 = 7.0f;
        m1 = calcPHBase(v1, t1);
        m2 = calcPHBase(v2, t2);
    } else {
        target1 = 0.0f;
        target2 = 8.0f; // Example target for DO
        m1 = calcDOBase(v1, t1);
        m2 = calcDOBase(v2, t2);
    }

    if (abs(m2 - m1) < 0.0001f) return false;

    alphaOut = (target2 - target1) / (m2 - m1);
    betaOut  = target1 - (alphaOut * m1);
    return true;
}

float SensorMath::calculate(float volt, float tempC, float alpha, float beta) {
    float ecFinal = calculateEC(volt, tempC, alpha, beta);
    
    if (ecFinal <= 0.0f) {//กรองกรณี EC ติดลบหรือเป็นศูนย์ (ซึ่งไม่สมเหตุสมผล) ให้คืนค่า Salinity เป็น 0.0 ppt แทน
        return 0.0f;
    }
    // แปลงจาก EC (mS/cm) เป็น Salinity (PPT)
    float salinity = (0.4803f * ecFinal) + 0.1634f;
    return max(0.0f, salinity);
}

float SensorMath::calculateEC(float volt, float tempC, float alpha, float beta) {
    float ec25_current = calcEC25(volt, tempC);
    float ecFinal = (alpha * ec25_current) + beta;
    return max(0.0f, ecFinal);
}

float SensorMath::calculatePH(float volt, float tempC, float alpha, float beta) {
    float phBase = calcPHBase(volt, tempC);
    float phFinal = (alpha * phBase) + beta;
    return constrain(phFinal, 0.0f, 14.0f);
}

float SensorMath::calculateDO(float volt, float tempC, float alpha, float beta) {
    float doBase = calcDOBase(volt, tempC);
    float doFinal = (alpha * doBase) + beta;
    return max(0.0f, doFinal);
}

bool SensorMath::captureStableValue(uint8_t currentParam, float &capturedVolt, float &capturedTemp) {
    const int N = 8;
    float bufBase[N];
    float sumVolt = 0.0f, sumTemp = 0.0f;
    
    for (int i = 0; i < N; i++) {
        SensorData s;
        xQueuePeek(sensorQueue, &s, 0); 
        float v = 0;
#if SENSOR_COUNT == 3
        if (currentParam == 0) v = s.voltEC;
        else if (currentParam == 1) v = s.voltPH;
        else v = s.voltDO;
#else
        v = s.voltEC;
#endif
        
        float baseVal = 0.0f;
        if (currentParam == 0) baseVal = calcEC25(v, s.tempC);
        else if (currentParam == 1) baseVal = calcPHBase(v, s.tempC);
        else baseVal = calcDOBase(v, s.tempC);
        
        bufBase[i] = baseVal;
        sumVolt += v;
        sumTemp += s.tempC;
        
        vTaskDelay(pdMS_TO_TICKS(80)); 
    }
    
    float minB = bufBase[0], maxB = bufBase[0];
    for (int i = 0; i < N; i++) {
        if (bufBase[i] < minB) minB = bufBase[i];
        if (bufBase[i] > maxB) maxB = bufBase[i];
    }
    
    float threshold = (currentParam == 1) ? 0.2f : 0.5f;
    if ((maxB - minB) > threshold) {
        return false; 
    }
    
    capturedVolt = sumVolt / N;
    capturedTemp = sumTemp / N;
    return true;
}
