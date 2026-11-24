#include "SensorMath.h"
#include "Shared.h"

// ==========================================
// Constants
// ==========================================
// const float SensorMath::TARGET_PPT = 6.35f;

// Helper ฟังก์ชันสำหรับคำนวณ DO Saturation ตามอุณหภูมิ (ลดการเขียนโค้ดซ้ำ)
static float getSaturationDO(float tempC) {
    return 14.46f - (0.38f * tempC) + (0.0054f * tempC * tempC);
}

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
    // percentDO = max(0.0f, percentDO); // ปลดล็อคให้แสดงค่า % ที่ติดลบได้เพื่อการตรวจเช็คสภาพเซ็นเซอร์
    float maxDO = getSaturationDO(tempC);
    return percentDO * maxDO; 
}

bool SensorMath::computeAlphaBeta(uint8_t currentParam, float v1, float t1, 
                                float v2, float t2, 
                                float &alphaOut, float &betaOut) {
    float m1 = 0, m2 = 0;
    float target1 = 0, target2 = 0;

    if (currentParam == 0) {
        target1 = 0.0f;
        target2 = 12.88f;
        m1 = calcEC25(v1, t1);
        m2 = calcEC25(v2, t2);
    } else if (currentParam == 1) {
        target1 = 4.01f; // ค่าน้ำยาขวดที่ 1
        target2 = 6.86f; // ค่าน้ำยาขวดที่ 2
        m1 = calcPHBase(v1, t1);
        m2 = calcPHBase(v2, t2);
    } else {
        target1 = 0.0f;
        // เป้าหมาย Std2 แปรผันตามอุณหภูมิน้ำขณะคาลิเบรตจุดที่ 2 (t2)
        target2 = getSaturationDO(t2);
        m1 = calcDOBase(v1, t1);
        m2 = calcDOBase(v2, t2);
    }

    if (fabsf(m2 - m1) < 0.000001f) return false; // ใช้ fabsf สำหรับทศนิยม float ป้องกัน Bug จากการแปลง Type

    alphaOut = (target2 - target1) / (m2 - m1);
    betaOut  = target1 - (alphaOut * m1);
    return true;
}

float SensorMath::calculate(float volt, float tempC, float alpha, float beta) {
    float ecFinal = calculateEC(volt, tempC, alpha, beta);
    
    // แปลงจาก EC (mS/cm) เป็น Salinity (PPT)
    float salinity = (0.4803f * ecFinal) + 0.1634f;
    // ปล่อยให้ Salinity คืนค่าติดลบได้เพื่อให้ผู้ใช้รู้ตัวว่าต้อง Calibrate
    return salinity;
}

float SensorMath::calculateEC(float volt, float tempC, float alpha, float beta) {
    float ec25_current = calcEC25(volt, tempC);
    float ecFinal = (alpha * ec25_current) + beta;
    return ecFinal;
}

float SensorMath::calculatePH(float volt, float tempC, float alpha, float beta) {
    float phBase = calcPHBase(volt, tempC);
    float phFinal = (alpha * phBase) + beta;
    return constrain(phFinal, -2.0f, 16.0f); // ขยายขอบเขตให้กว้างขึ้น เพื่อให้เห็นระยะ Drift
}

float SensorMath::calculateDO(float volt, float tempC, float alpha, float beta) {
    float doBase = calcDOBase(volt, tempC);
    float doFinal = (alpha * doBase) + beta;
    return doFinal;
}

// ==========================================
// Stability Tracker
// ==========================================
// Ring buffer ต่อเซ็นเซอร์ (เผื่อไว้ 3 ช่องเสมอ รุ่น 1 หัวใช้แค่ช่อง 0)
struct StabBuf {
    float   base[SensorMath::STABLE_WINDOW];
    float   volt[SensorMath::STABLE_WINDOW];
    float   temp[SensorMath::STABLE_WINDOW];
    uint8_t head;
    uint8_t count;
};
static StabBuf      s_stab[3] = {};
static portMUX_TYPE s_stabMux = portMUX_INITIALIZER_UNLOCKED; // กันอ่าน/เขียนชนกันข้าม Core

float SensorMath::stableThreshold(uint8_t param) {
    if (param == 1) return 0.03f;  // pH: แกว่งไม่เกิน 0.03 pH
    return 0.5f;                   // EC (mS/cm) / DO (mg/L): คงเกณฑ์เดิม
}

void SensorMath::pushSample(uint8_t param, float volt, float tempC) {
    if (param > 2) return;

    float baseVal;
    if (param == 0)      baseVal = calcEC25(volt, tempC);
    else if (param == 1) baseVal = calcPHBase(volt, tempC);
    else                 baseVal = calcDOBase(volt, tempC);

    portENTER_CRITICAL(&s_stabMux);
    StabBuf& b = s_stab[param];
    b.base[b.head] = baseVal;
    b.volt[b.head] = volt;
    b.temp[b.head] = tempC;
    b.head = (b.head + 1) % STABLE_WINDOW;
    if (b.count < STABLE_WINDOW) b.count++;
    portEXIT_CRITICAL(&s_stabMux);
}

void SensorMath::clearStability() {
    portENTER_CRITICAL(&s_stabMux);
    for (auto& b : s_stab) { b.head = 0; b.count = 0; }
    portEXIT_CRITICAL(&s_stabMux);
}

// คัดลอก buffer ออกมาก่อน แล้วค่อยคำนวณนอก critical section
static StabBuf _snapshot(uint8_t param) {
    StabBuf copy;
    portENTER_CRITICAL(&s_stabMux);
    copy = s_stab[param];
    portEXIT_CRITICAL(&s_stabMux);
    return copy;
}

SensorMath::StabilityInfo SensorMath::getStability(uint8_t param) {
    StabilityInfo info{0, 0.0f, false};
    if (param > 2) return info;

    StabBuf b = _snapshot(param);
    info.count = b.count;
    if (b.count == 0) return info;

    float minB = b.base[0], maxB = b.base[0];
    for (uint8_t i = 1; i < b.count; i++) {
        if (b.base[i] < minB) minB = b.base[i];
        if (b.base[i] > maxB) maxB = b.base[i];
    }
    info.range  = maxB - minB;
    info.stable = (b.count >= STABLE_WINDOW) && (info.range <= stableThreshold(param));
    return info;
}

bool SensorMath::captureStableValue(uint8_t currentParam, float &capturedVolt, float &capturedTemp) {
    if (currentParam > 2) return false;

    StabBuf b = _snapshot(currentParam);
    if (b.count < STABLE_WINDOW) return false;   // ยังเก็บค่าไม่ครบหน้าต่าง

    float minB = b.base[0], maxB = b.base[0];
    float sumVolt = 0.0f, sumTemp = 0.0f;
    for (uint8_t i = 0; i < STABLE_WINDOW; i++) {
        if (b.base[i] < minB) minB = b.base[i];
        if (b.base[i] > maxB) maxB = b.base[i];
        sumVolt += b.volt[i];
        sumTemp += b.temp[i];
    }

    if ((maxB - minB) > stableThreshold(currentParam)) {
        return false;                            // ยังแกว่งเกินเกณฑ์
    }

    capturedVolt = sumVolt / STABLE_WINDOW;
    capturedTemp = sumTemp / STABLE_WINDOW;
    return true;
}
