#pragma once
#include <Arduino.h>
#include <Preferences.h>

// ข้อมูล Calibration
struct CalibData {
    float v_di   = 0.0004f;  // แรงดัน DI water
    float v_salt = 0.4746f;  // แรงดัน 35ppt solution
    float t_salt = 23.50f;   // อุณหภูมิตอน calibrate

    float alpha = 1.0f; 
    float beta  = 0.0f;
};

// ข้อมูล Threshold
struct ThreshData {
    float green = 15.0f;  // ต่ำกว่านี้ → LED เขียว
    float yellow= 25.0f;  // ต่ำกว่านี้ → LED เหลือง + alarm
    float red   = 30.0f;  // สูงกว่านี้ → LED แดง + alarm
};

struct SystemConfig {
    uint8_t networkMode = 0; // 0 = SIM, 1 = WIFI
    bool isMuted = false;        // true = ปิดเสียง (Mute), false = เปิดเสียง
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
    static CalibData  calib;
    static ThreshData thresh;
    static SystemConfig config;

private:
    static Preferences _prefs;
    static const char* NAMESPACE;  // "salinity"

    // Key names รวมไว้ที่เดียว ป้องกันพิมพ์ผิด
    static const char* KEY_V_DI;
    static const char* KEY_V_SALT;
    static const char* KEY_T_SALT;
    static const char* KEY_THRESH_G;
    static const char* KEY_THRESH_Y;
    static const char* KEY_THRESH_R;
    static const char* KEY_ALPHA;
    static const char* KEY_BETA;
    static const char* KEY_NET_MODE;
    static const char* KEY_IS_MUTED;
};