#pragma once
#include <Arduino.h>
#include "shared.h"
#include "NVSManager.h"
#include "SoundManager.h"
#include "WifiTask.h"

// ทุก State ของโปรแกรม
enum class AppState {
    STARTUP,
    MAIN_SCREEN,
    MAIN_MENU,
    READ_TEMP,
    READ_GPS,

#if SENSOR_COUNT == 3
    // State หน้าจอคั่นกลาง สำหรับเมนู 3 หัว
    MONITOR_MENU,
    THRESH_PARAM_MENU,
    CAL_PARAM_MENU,
#endif

    THRESH_MENU,
    EDIT_THRESH,
    CAL_MENU, 
    TEMP_CAL,
    SYSTEM_INFO,
    CAL_MANUAL, 
    EDIT_CAL_MANUAL, 
    CAL_DI,
    CAL_SALT,
    CAL_FINISH,
    CAL_CANCEL_CONFIRM,
    SIM_SENDING,
    SIM_RESULT,
    NETWORK_STATUS,
    SYSTEM_SETUP,
};

class StateMachine {
public:
    StateMachine();

    // เรียกจาก DisplayTask ทุก loop
    void handleEvent(ButtonEvent ev);
    void begin();

    // State ปัจจุบัน
    AppState getState()  const { return _current; }
    AppState getPrev()   const { return _prev; }
    bool     hasChanged();  // true ครั้งแรกที่อ่าน แล้ว reset

    // ข้อมูลที่ ScreenManager ต้องการ
    int  menuIndex      = 0;
    int  cancelSelect   = 1;   // 0=YES, 1=NO
    char editingColor   = 'G'; // 'G' หรือ 'R' หรือ 'A', 'B' (สำหรับ Calibrate)

#if SENSOR_COUNT == 3
    // ตัวแปรจำว่ากำลังเลือกเซ็นเซอร์ตัวไหนอยู่ (0 = EC, 1 = pH, 2 = DO)
    uint8_t currentParam = 0; 
#endif

    // ค่า calibration ชั่วคราว (ก่อนกด confirm)
    float tmp_v_di   = 0.0f;
    float tmp_t_di   = 0.0f;
    float tmp_v_salt = 0.0f;
    float tmp_t_salt = 0.0f;

    bool calibSuccess = false; // ตัวแปรบอกสถานะว่าคำนวณผ่านหรือไม่
    bool simLastSuccess  = false;
    int  simLastHttpCode = 0;

    // SimTask เรียกเมื่อส่งเสร็จ
    void onSimSendComplete(bool success, int httpCode);

private:
    AppState _current;
    AppState _prev;
    AppState _returnState;   // จำไว้ว่าต้องกลับไปหน้าไหนหลังส่งเสร็จ
    AppState _lastCalState;  // จำไว้ว่าอยู่ขั้นไหนก่อน cancel
    bool     _changed;

    void _goTo(AppState next);
    
    // Handler แต่ละ State (ใช้ร่วมกัน)
    void _handleMainScreen(ButtonEvent ev);
    void _handleMainMenu(ButtonEvent ev);
    void _handleReadTemp(ButtonEvent ev);
    void _handleReadGPS(ButtonEvent ev);
    void _handleThreshMenu(ButtonEvent ev);
    void _handleEditThresh(ButtonEvent ev);
    void _handleCalMenu(ButtonEvent ev); 
    void _handleTempCal(ButtonEvent ev);
    void _handleSystemInfo(ButtonEvent ev);
    void _handleCalManual(ButtonEvent ev, const SensorData& sensor);
    void _handleEditCalManual(ButtonEvent ev);
    void _handleCalDI(ButtonEvent ev, const SensorData& sensor);
    void _handleCalSalt(ButtonEvent ev, const SensorData& sensor);
    void _handleCalCancelConfirm(ButtonEvent ev);
    void _handleSimStatus(ButtonEvent ev);
    void _handleSimSending(ButtonEvent ev);
    void _handleSimResult(ButtonEvent ev);
    void _handleNetworkStatus(ButtonEvent ev); 
    void _handleSystemSetup(ButtonEvent ev); 

#if SENSOR_COUNT == 3
    // Handler ที่มีเฉพาะรุ่น 3 หัว
    void _handleMonitorMenu(ButtonEvent ev);
    void _handleThreshParamMenu(ButtonEvent ev);
    void _handleCalParamMenu(ButtonEvent ev);
#endif
};