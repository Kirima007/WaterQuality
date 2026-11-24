#include "NVSManager.h"
#include "config.h"

// ==========================================
// Static Member Definitions
// ==========================================
CalibData  NVSManager::calibEC;
#if SENSOR_COUNT == 3
CalibData  NVSManager::calibPH;
CalibData  NVSManager::calibDO;
ThreshData NVSManager::threshEC;
ThreshData NVSManager::threshPH;
ThreshData NVSManager::threshDO;
#else
ThreshData NVSManager::thresh;
#endif
Preferences NVSManager::_prefs;
SystemConfig NVSManager::config;
float NVSManager::tempOffset = 0.0f;

// Key names รวมไว้ที่เดียว
// ถ้าอยากเปลี่ยน key แก้ที่นี่ที่เดียวพอ
#if SENSOR_COUNT == 3
const char* NVSManager::NAMESPACE    = "sys_data";
#else
const char* NVSManager::NAMESPACE    = "salinity"; // เปลี่ยนกลับไปชี้ที่แฟ้มเก่า
#endif

#if SENSOR_COUNT == 3
const char* NVSManager::KEY_EC_A     = "ec_a";
const char* NVSManager::KEY_EC_B     = "ec_b";
const char* NVSManager::KEY_PH_A     = "ph_a";
const char* NVSManager::KEY_PH_B     = "ph_b";
const char* NVSManager::KEY_DO_A     = "do_a";
const char* NVSManager::KEY_DO_B     = "do_b";

const char* NVSManager::KEY_TH_EC_G = "th_ec_g";
const char* NVSManager::KEY_TH_EC_Y = "th_ec_y";
const char* NVSManager::KEY_TH_EC_R = "th_ec_r";
const char* NVSManager::KEY_TH_PH_G = "th_ph_g";
const char* NVSManager::KEY_TH_PH_Y = "th_ph_y";
const char* NVSManager::KEY_TH_PH_R = "th_ph_r";
const char* NVSManager::KEY_TH_DO_G = "th_do_g";
const char* NVSManager::KEY_TH_DO_Y = "th_do_y";
const char* NVSManager::KEY_TH_DO_R = "th_do_r";
#else
const char* NVSManager::KEY_ALPHA    = "alpha";
const char* NVSManager::KEY_BETA     = "beta";
const char* NVSManager::KEY_THRESH_G = "th_g";
const char* NVSManager::KEY_THRESH_Y = "th_y";
const char* NVSManager::KEY_THRESH_R = "th_r";
#endif

const char* NVSManager::KEY_NET_MODE  = "net_mode"; // ← เพิ่ม
const char* NVSManager::KEY_IS_MUTED  = "is_muted"; // ← เพิ่ม
const char* NVSManager::KEY_TEMP_OFF  = "tmp_off";
const char* NVSManager::KEY_DEV_ID    = "dev_id";


// ==========================================
// Load — เรียกครั้งเดียวตอน boot
// ==========================================
// ตารางค่า float ใน NVS: key, ตัวแปรปลายทาง, ค่าเริ่มต้น
struct FloatEntry {
    const char* key;
    float*      target;
    float       def;
};

void NVSManager::load() {
    // ค่าเริ่มต้นทั้งหมดอยู่ในตารางนี้ที่เดียว (ใช้ทั้งตอนเติม key ที่ขาด และตอนโหลด)
    const FloatEntry floats[] = {
#if SENSOR_COUNT == 3
        // Calibration (Default alpha = 1.0, beta = 0.0)
        { KEY_EC_A,    &calibEC.alpha,   1.0f },
        { KEY_EC_B,    &calibEC.beta,    0.0f },
        { KEY_PH_A,    &calibPH.alpha,   1.0f },
        { KEY_PH_B,    &calibPH.beta,    0.0f },
        { KEY_DO_A,    &calibDO.alpha,   1.0f },
        { KEY_DO_B,    &calibDO.beta,    0.0f },
        // Thresholds สำหรับ 3 หัว
        { KEY_TH_EC_G, &threshEC.green,  1.0f },
        { KEY_TH_EC_Y, &threshEC.yellow, 2.5f },
        { KEY_TH_EC_R, &threshEC.red,    4.0f },
        { KEY_TH_PH_G, &threshPH.green,  6.5f },
        { KEY_TH_PH_Y, &threshPH.yellow, 8.5f },
        { KEY_TH_PH_R, &threshPH.red,    9.0f },
        { KEY_TH_DO_G, &threshDO.green,  5.0f },
        { KEY_TH_DO_Y, &threshDO.yellow, 4.0f }, // DO เป็นแบบ "ต่ำกว่าแล้วเตือน"
        { KEY_TH_DO_R, &threshDO.red,    3.0f },
#else
        // Calibration (1CH)
        { KEY_ALPHA,    &calibEC.alpha,  1.0f },
        { KEY_BETA,     &calibEC.beta,   0.0f },
        // Threshold (1CH)
        { KEY_THRESH_G, &thresh.green,   1.0f },
        { KEY_THRESH_Y, &thresh.yellow,  2.5f },
        { KEY_THRESH_R, &thresh.red,     4.0f },
#endif
        // Temp Offset
        { KEY_TEMP_OFF, &tempOffset,     0.0f },
    };
    const size_t floatCount = sizeof(floats) / sizeof(floats[0]);

    // 1. เติมค่าเริ่มต้นให้ key float ที่ยังไม่เคยถูกบันทึก (เขียนครั้งเดียว ค่าที่ save ไว้แล้วไม่ถูกแตะ)
    //    ถ้าไม่เติม getFloat() จะพิมพ์ log error "NOT_FOUND" ทุกครั้งที่โหลด
    //    (isKey() ไม่พิมพ์ log)
    _prefs.begin(NAMESPACE, true);
    bool missing = false;
    for (size_t i = 0; i < floatCount; i++) {
        if (!_prefs.isKey(floats[i].key)) { missing = true; break; }
    }
    _prefs.end();

    if (missing) {
        _prefs.begin(NAMESPACE, false); // Read/Write (สร้าง namespace ให้ถ้ายังไม่มี)
        for (size_t i = 0; i < floatCount; i++) {
            if (!_prefs.isKey(floats[i].key)) {
                _prefs.putFloat(floats[i].key, floats[i].def);
            }
        }
        _prefs.end();
    }

    // 2. โหลดค่าทั้งหมด
    _prefs.begin(NAMESPACE, true); // true = Read Only

    for (size_t i = 0; i < floatCount; i++) {
        *floats[i].target = _prefs.getFloat(floats[i].key, floats[i].def);
    }

    // โหลด System Config (getter ชนิดตัวเลข/bool ไม่พิมพ์ log error เมื่อหา key ไม่เจอ)
    config.networkMode = _prefs.getUInt(KEY_NET_MODE, 0);
    config.isMuted = _prefs.getBool(KEY_IS_MUTED, false);
    config.deviceId = _prefs.getUShort(KEY_DEV_ID, DEVICE_ID);

    _prefs.end();

    // Sanity check, constrain to the same range as in the UI (-10.0 to 10.0)
    if (tempOffset < -10.0f || tempOffset > 10.0f) {
        tempOffset = 0.0f;
    }
}

// ==========================================
// Save Calibration
// ==========================================
void NVSManager::saveCalib() {
    _prefs.begin(NAMESPACE, false); // false = Read/Write

#if SENSOR_COUNT == 3
    _prefs.putFloat(KEY_EC_A, calibEC.alpha);
    _prefs.putFloat(KEY_EC_B, calibEC.beta);

    _prefs.putFloat(KEY_PH_A, calibPH.alpha);
    _prefs.putFloat(KEY_PH_B, calibPH.beta);
    
    _prefs.putFloat(KEY_DO_A, calibDO.alpha);
    _prefs.putFloat(KEY_DO_B, calibDO.beta);
#else
    _prefs.putFloat(KEY_ALPHA, calibEC.alpha);
    _prefs.putFloat(KEY_BETA, calibEC.beta);
#endif

    _prefs.putFloat(KEY_TEMP_OFF, tempOffset);

    _prefs.end();
}

// ==========================================
// Save Threshold
// ==========================================
void NVSManager::saveThresh() {
    _prefs.begin(NAMESPACE, false);
#if SENSOR_COUNT == 3
    _prefs.putFloat(KEY_TH_EC_G, threshEC.green);
    _prefs.putFloat(KEY_TH_EC_Y, threshEC.yellow);
    _prefs.putFloat(KEY_TH_EC_R, threshEC.red);

    _prefs.putFloat(KEY_TH_PH_G, threshPH.green);
    _prefs.putFloat(KEY_TH_PH_Y, threshPH.yellow);
    _prefs.putFloat(KEY_TH_PH_R, threshPH.red);

    _prefs.putFloat(KEY_TH_DO_G, threshDO.green);
    _prefs.putFloat(KEY_TH_DO_Y, threshDO.yellow);
    _prefs.putFloat(KEY_TH_DO_R, threshDO.red);
#else
    _prefs.putFloat(KEY_THRESH_G, thresh.green);
    _prefs.putFloat(KEY_THRESH_Y, thresh.yellow);
    _prefs.putFloat(KEY_THRESH_R, thresh.red);
#endif
    _prefs.end();
}

// ==========================================
// Save Config
// ==========================================
void NVSManager::saveConfig() {
    _prefs.begin(NAMESPACE, false);
    _prefs.putUInt(KEY_NET_MODE, config.networkMode);
    _prefs.putBool(KEY_IS_MUTED, config.isMuted);
    _prefs.putUShort(KEY_DEV_ID, config.deviceId);
    _prefs.end();
}

// ==========================================
// Reset — คืนค่า default ทั้งหมด (ยกเว้น Calib, Temp Offset และ Device ID)
// ==========================================
void NVSManager::reset() {
    // 1. เก็บค่า Calibration และ Device ID เอาไว้ก่อน
    CalibData savedCalibEC = calibEC;
#if SENSOR_COUNT == 3
    CalibData savedCalibPH = calibPH;
    CalibData savedCalibDO = calibDO;
    
    // คืนค่า Threshold กลับเป็น Default
    threshEC = ThreshData{};
    threshPH = ThreshData{};
    threshDO = ThreshData{};
#else
    thresh = ThreshData{};
#endif

    uint16_t savedDeviceId = config.deviceId;
    float savedTempOffset = tempOffset;

    config = SystemConfig{}; // <-- เผื่ออยากให้ networkMode กลับเป็น 0 ด้วย

    // ลบทุกค่าใน namespace นี้ออกจาก Flash
    _prefs.begin(NAMESPACE, false);
    _prefs.clear();
    _prefs.end();

    // 2. คืนค่าที่ตั้งใจเก็บรักษาไว้
    calibEC = savedCalibEC;
#if SENSOR_COUNT == 3
    calibPH = savedCalibPH;
    calibDO = savedCalibDO;
#endif
    config.deviceId = savedDeviceId;
    tempOffset = savedTempOffset;

    // 3. เซฟค่าทั้งหมดกลับลง Flash ทันที
    saveCalib();
    saveThresh();
    saveConfig();
}