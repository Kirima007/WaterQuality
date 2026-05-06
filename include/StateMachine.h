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
    THRESH_MENU,
    EDIT_THRESH,
    CAL_MENU, // new
    SYSTEM_INFO,
    CAL_MANUAL, // new
    EDIT_CAL_MANUAL, // new
    CAL_DI,
    CAL_SALT,
    CAL_FINISH,
    CAL_CANCEL_CONFIRM,
    // SIM_STATUS,
    SIM_SENDING,
    SIM_RESULT,
    NETWORK_STATUS,
    NETWORK_MENU, // new
    BUZZER_MENU, // new
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
    char editingColor   = 'G'; // 'G' หรือ 'R'

    // ค่า calibration ชั่วคราว (ก่อนกด confirm)
    float tmp_v_di   = 0.0f;
    float tmp_t_di   = 0.0f;
    float tmp_v_salt = 0.0f;
    float tmp_t_salt = 0.0f;

    bool simLastSuccess  = false;
    int  simLastHttpCode = 0;

    // SimTask เรียกเมื่อส่งเสร็จ
    void onSimSendComplete(bool success, int httpCode);


private:
    AppState _current;
    AppState _prev;
    AppState _lastCalState;  // จำไว้ว่าอยู่ขั้นไหนก่อน cancel
    bool     _changed;

    void _goTo(AppState next);

    bool _captureStableValue(float &capturedVolt, float &capturedTemp);
    // Handler แต่ละ State
    void _handleMainScreen(ButtonEvent ev);
    void _handleMainMenu(ButtonEvent ev);
    void _handleReadTemp(ButtonEvent ev);
    void _handleReadGPS(ButtonEvent ev);
    void _handleThreshMenu(ButtonEvent ev);
    void _handleEditThresh(ButtonEvent ev);
    void _handleCalMenu(ButtonEvent ev); // new
    void _handleSystemInfo(ButtonEvent ev);
    void _handleCalManual(ButtonEvent ev, const SensorData& sensor);
    void _handleEditCalManual(ButtonEvent ev);
    void _handleCalDI(ButtonEvent ev, const SensorData& sensor);
    void _handleCalSalt(ButtonEvent ev, const SensorData& sensor);
    void _handleCalFinish();
    void _handleCalCancelConfirm(ButtonEvent ev);
    void _handleSimStatus(ButtonEvent ev);
    void _handleSimSending(ButtonEvent ev);
    void _handleSimResult(ButtonEvent ev);
    void _handleNetworkMenu(ButtonEvent ev);
    void _handleNetworkStatus(ButtonEvent ev); // new
    void _handleBuzzerMenu(ButtonEvent ev); // new
};