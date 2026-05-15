#pragma once
#include <Arduino.h>
#include <Preferences.h>

// ข้อมูล Calibration
struct CalibData {
    float alpha = 1.0f;
    float beta = 0.0f;
};

// ข้อมูล Threshold
struct ThreshData {
    float green = 15.0f;  // ต่ำกว่านี้ → LED เขียว
    float yellow= 25.0f;  // ต่ำกว่านี้ → LED เหลือง + alarm
    float red   = 30.0f;  // สูงกว่านี้ → LED แดง + alarm
};

struct SystemConfig {
    uint8_t networkMode = 0;        // 0 = SIM, 1 = WIFI
    bool isMuted = false;           // true = ปิดเสียง (Mute), false = เปิดเสียง
};
class NVSManager {
public:
    // โหลดค่าทั้งหมดจาก Flash ตอน boot
    static void load();

    // บันทึก calibration ลง Flash
    static void saveCalib();

    // บันทึก threshold ลง Flash
    static void saveThresh();

    static void saveConfig();

    // รีเซ็ตกลับค่า default ทั้งหมด
    static void reset();

    // ข้อมูลที่ Task อื่นเข้าถึงได้โดยตรง
    static CalibData calibEC;
#if SENSOR_COUNT == 3
    static CalibData calibPH;
    static CalibData calibDO;

    static ThreshData threshEC;
    static ThreshData threshPH;
    static ThreshData threshDO;
#else
    static ThreshData thresh;
#endif
    static SystemConfig config;

    static float tempOffset;
private:
    static Preferences _prefs;
    static const char* NAMESPACE;  // "salinity"

#if SENSOR_COUNT == 3
    // Key names รวมไว้ที่เดียว ป้องกันพิมพ์ผิด
    static const char* KEY_EC_A;
    static const char* KEY_EC_B;

    static const char* KEY_PH_A;
    static const char* KEY_PH_B;
    static const char* KEY_DO_A;
    static const char* KEY_DO_B;

    static const char* KEY_TH_EC_G;
    static const char* KEY_TH_EC_Y;
    static const char* KEY_TH_EC_R;
    static const char* KEY_TH_PH_G;
    static const char* KEY_TH_PH_Y;
    static const char* KEY_TH_PH_R;
    static const char* KEY_TH_DO_G;
    static const char* KEY_TH_DO_Y;
    static const char* KEY_TH_DO_R;
#else
    static const char* KEY_ALPHA;
    static const char* KEY_BETA;
    static const char* KEY_THRESH_G;
    static const char* KEY_THRESH_Y;
    static const char* KEY_THRESH_R;
#endif

    static const char* KEY_NET_MODE;
    static const char* KEY_IS_MUTED;
    static const char* KEY_TEMP_OFF;
};