#include "NVSManager.h"

// ==========================================
// Static Member Definitions
// ==========================================
CalibData  NVSManager::calibEC;
#if SENSOR_COUNT == 3
CalibData  NVSManager::calibPH;
CalibData  NVSManager::calibDO;
#endif
ThreshData NVSManager::thresh;
Preferences NVSManager::_prefs;
SystemConfig NVSManager::config;

// Key names รวมไว้ที่เดียว
// ถ้าอยากเปลี่ยน key แก้ที่นี่ที่เดียวพอ
const char* NVSManager::NAMESPACE    = "sys_data";
const char* NVSManager::KEY_EC_A     = "ec_a";
const char* NVSManager::KEY_EC_B     = "ec_b";

#if SENSOR_COUNT == 3
const char* NVSManager::KEY_PH_A     = "ph_a";
const char* NVSManager::KEY_PH_B     = "ph_b";
const char* NVSManager::KEY_DO_A     = "do_a";
const char* NVSManager::KEY_DO_B     = "do_b";
#endif

const char* NVSManager::KEY_THRESH_G = "th_g";
const char* NVSManager::KEY_THRESH_Y = "th_y";
const char* NVSManager::KEY_THRESH_R = "th_r";

const char* NVSManager::KEY_NET_MODE  = "net_mode"; // ← เพิ่ม
const char* NVSManager::KEY_IS_MUTED  = "is_muted"; // ← เพิ่ม


// ==========================================
// Load — เรียกครั้งเดียวตอน boot
// ==========================================
void NVSManager::load() {
    _prefs.begin(NAMESPACE, true); // true = Read Only

    // 1. โหลดค่า EC (ใช้ตัวเลขบนหัวโพรบ 0.99x เป็นค่า Default ได้เลย)
    calibEC.alpha = _prefs.getFloat(KEY_EC_A, 1.0f); 
    calibEC.beta  = _prefs.getFloat(KEY_EC_B, 0.0f);

#if SENSOR_COUNT == 3
    // 2. โหลดค่า pH และ DO (Default alpha = 1.0)
    calibPH.alpha = _prefs.getFloat(KEY_PH_A, 1.0f);
    calibPH.beta  = _prefs.getFloat(KEY_PH_B, 0.0f);

    calibDO.alpha = _prefs.getFloat(KEY_DO_A, 1.0f);
    calibDO.beta  = _prefs.getFloat(KEY_DO_B, 0.0f);
#endif


    // โหลด Threshold
    thresh.green = _prefs.getFloat(KEY_THRESH_G, thresh.green);
    thresh.yellow = _prefs.getFloat(KEY_THRESH_Y, thresh.yellow);
    thresh.red   = _prefs.getFloat(KEY_THRESH_R, thresh.red);

    // โหลด System Config
    config.networkMode = _prefs.getUInt(KEY_NET_MODE, config.networkMode);
    config.isMuted = _prefs.getBool(KEY_IS_MUTED, config.isMuted);

    _prefs.end();
}

// ==========================================
// Save Calibration
// ==========================================
void NVSManager::saveCalib() {
    _prefs.begin(NAMESPACE, false); // false = Read/Write

    _prefs.putFloat(KEY_EC_A, calibEC.alpha);
    _prefs.putFloat(KEY_EC_B, calibEC.beta);

#if SENSOR_COUNT == 3
    _prefs.putFloat(KEY_PH_A, calibPH.alpha);
    _prefs.putFloat(KEY_PH_B, calibPH.beta);
    
    _prefs.putFloat(KEY_DO_A, calibDO.alpha);
    _prefs.putFloat(KEY_DO_B, calibDO.beta);
#endif

    _prefs.end();
}

// ==========================================
// Save Threshold
// ==========================================
void NVSManager::saveThresh() {
    _prefs.begin(NAMESPACE, false);
    _prefs.putFloat(KEY_THRESH_G, thresh.green);
    _prefs.putFloat(KEY_THRESH_Y, thresh.yellow);
    _prefs.putFloat(KEY_THRESH_R, thresh.red);
    _prefs.end();
}

// ==========================================
// Save Config
// ==========================================
void NVSManager::saveConfig() {
    _prefs.begin(NAMESPACE, false);
    _prefs.putUInt(KEY_NET_MODE, config.networkMode);
    _prefs.putBool(KEY_IS_MUTED, config.isMuted);
    _prefs.end();
}

// ==========================================
// Reset — คืนค่า default ทั้งหมด
// ==========================================
void NVSManager::reset() {
    // คืนค่า struct กลับ default
    calibEC = CalibData{};
#if SENSOR_COUNT == 3
    calibPH = CalibData{};
    calibDO = CalibData{};
#endif

    thresh = ThreshData{};
    config = SystemConfig{}; // <-- เผื่ออยากให้ networkMode กลับเป็น 0 ด้วย

    // ลบทุกค่าใน namespace นี้ออกจาก Flash
    _prefs.begin(NAMESPACE, false);
    _prefs.clear();
    _prefs.end();
}