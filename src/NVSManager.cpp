#include "NVSManager.h"

// ==========================================
// Static Member Definitions
// ==========================================
CalibData  NVSManager::calib;
ThreshData NVSManager::thresh;
Preferences NVSManager::_prefs;

// Key names รวมไว้ที่เดียว
// ถ้าอยากเปลี่ยน key แก้ที่นี่ที่เดียวพอ
const char* NVSManager::NAMESPACE   = "salinity";
const char* NVSManager::KEY_V_DI    = "v_di";
const char* NVSManager::KEY_V_SALT  = "v_salt";
const char* NVSManager::KEY_T_SALT  = "t_salt";
const char* NVSManager::KEY_THRESH_G = "th_g";
const char* NVSManager::KEY_THRESH_R = "th_r";
const char* NVSManager::KEY_ALPHA    = "alpha";
const char* NVSManager::KEY_BETA     = "beta";

// ==========================================
// Load — เรียกครั้งเดียวตอน boot
// ==========================================
void NVSManager::load() {
    _prefs.begin(NAMESPACE, false);  // false = read/write mode

    // โหลด Calibration (ถ้าไม่มีใน Flash ใช้ค่า default)
    calib.v_di   = _prefs.getFloat(KEY_V_DI,    calib.v_di);
    calib.v_salt = _prefs.getFloat(KEY_V_SALT,   calib.v_salt);
    calib.t_salt = _prefs.getFloat(KEY_T_SALT,   calib.t_salt);
    calib.alpha  = _prefs.getFloat(KEY_ALPHA,    calib.alpha); // เซฟ Alpha/Beta ด้วย
    calib.beta   = _prefs.getFloat(KEY_BETA,     calib.beta);

    // โหลด Threshold
    thresh.green = _prefs.getFloat(KEY_THRESH_G, thresh.green);
    thresh.red   = _prefs.getFloat(KEY_THRESH_R, thresh.red);

    _prefs.end();
}

// ==========================================
// Save Calibration
// ==========================================
void NVSManager::saveCalib() {
    _prefs.begin(NAMESPACE, false);
    _prefs.putFloat(KEY_V_DI,   calib.v_di);
    _prefs.putFloat(KEY_V_SALT, calib.v_salt);
    _prefs.putFloat(KEY_T_SALT, calib.t_salt);
    _prefs.putFloat(KEY_ALPHA,  calib.alpha); // เซฟ Alpha/Beta ด้วย
    _prefs.putFloat(KEY_BETA,   calib.beta);
    _prefs.end();
}

// ==========================================
// Save Threshold
// ==========================================
void NVSManager::saveThresh() {
    _prefs.begin(NAMESPACE, false);
    _prefs.putFloat(KEY_THRESH_G, thresh.green);
    _prefs.putFloat(KEY_THRESH_R, thresh.red);
    _prefs.end();
}

// ==========================================
// Reset — คืนค่า default ทั้งหมด
// ==========================================
void NVSManager::reset() {
    // คืนค่า struct กลับ default
    calib  = CalibData{};
    thresh = ThreshData{};

    // ลบทุกค่าใน namespace นี้ออกจาก Flash
    _prefs.begin(NAMESPACE, false);
    _prefs.clear();
    _prefs.end();
}