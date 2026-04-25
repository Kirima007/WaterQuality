#include "StateMachine.h"

// ==========================================
// Constructor
// ==========================================
StateMachine::StateMachine() {
    _current      = AppState::STARTUP;
    _prev         = AppState::STARTUP;
    _lastCalState = AppState::CAL_DI;
    _changed      = true;
}



void StateMachine::begin() {
    _goTo(AppState::MAIN_SCREEN);
}

// ==========================================
// hasChanged — true ครั้งแรกที่อ่าน แล้ว reset
// ==========================================
bool StateMachine::hasChanged() {
    if (_changed) {
        _changed = false;
        return true;
    }
    return false;
}

// ==========================================
// _goTo — เปลี่ยน State
// ==========================================
void StateMachine::_goTo(AppState next) {
    _prev    = _current;
    _current = next;
    _changed = true;
}

// ==========================================
// handleEvent — จุดเข้าหลัก
// ==========================================
void StateMachine::handleEvent(ButtonEvent ev) {
    // ดึง sensor data ล่าสุด (สำหรับ CAL state)
    SensorData sensor{};
    xQueuePeek(sensorQueue, &sensor, 0);

    switch (_current) {
        case AppState::MAIN_SCREEN:         _handleMainScreen(ev);              break;
        case AppState::MAIN_MENU:           _handleMainMenu(ev);                break;
        case AppState::READ_TEMP:           _handleReadTemp(ev);                break;
        case AppState::READ_GPS:            _handleReadGPS(ev);                 break;
        case AppState::THRESH_MENU:         _handleThreshMenu(ev);              break;
        case AppState::EDIT_THRESH:         _handleEditThresh(ev);              break;
        case AppState::CAL_DI:              _handleCalDI(ev, sensor);           break;
        case AppState::CAL_SALT:            _handleCalSalt(ev, sensor);         break;
        case AppState::CAL_FINISH:          _handleCalFinish();                 break;
        case AppState::CAL_CANCEL_CONFIRM:  _handleCalCancelConfirm(ev);        break;
        default: break;
    }
}

// ==========================================
// MAIN_SCREEN
// ==========================================
void StateMachine::_handleMainScreen(ButtonEvent ev) {
    if (ev == ButtonEvent::SHORT_PRESS) {
        menuIndex = 0;
        _goTo(AppState::MAIN_MENU);
    }
}

// ==========================================
// MAIN_MENU
// ==========================================
void StateMachine::_handleMainMenu(ButtonEvent ev) {
    if (ev == ButtonEvent::ROTATE_CW) {
        menuIndex = (menuIndex + 1) % 5;
        requestSound(SoundEvent::SCROLL);
    } else if (ev == ButtonEvent::ROTATE_CCW) {
        menuIndex = (menuIndex - 1 + 5) % 5;
        requestSound(SoundEvent::SCROLL);
    } else if (ev == ButtonEvent::SHORT_PRESS) {
        switch (menuIndex) {
            case 0: _goTo(AppState::READ_TEMP);   break;
            case 1: _goTo(AppState::READ_GPS);    break;
            case 2:
                menuIndex = 0;
                _goTo(AppState::THRESH_MENU);
                break;
            case 3: _goTo(AppState::CAL_DI);      break;
            case 4: _goTo(AppState::MAIN_SCREEN);  break;
        }
    }
}

// ==========================================
// READ_TEMP / READ_GPS
// ==========================================
void StateMachine::_handleReadTemp(ButtonEvent ev) {
    if (ev == ButtonEvent::SHORT_PRESS) {
        menuIndex = 0;
        _goTo(AppState::MAIN_MENU);
    }
}

void StateMachine::_handleReadGPS(ButtonEvent ev) {
    if (ev == ButtonEvent::SHORT_PRESS) {
        menuIndex = 1;
        _goTo(AppState::MAIN_MENU);
    }
}

// ==========================================
// THRESH_MENU
// ==========================================
void StateMachine::_handleThreshMenu(ButtonEvent ev) {
    if (ev == ButtonEvent::ROTATE_CW) {
        requestSound(SoundEvent::SCROLL);
        menuIndex = (menuIndex + 1) % 4;
    } else if (ev == ButtonEvent::ROTATE_CCW) {
        requestSound(SoundEvent::SCROLL);
        menuIndex = (menuIndex - 1 + 4) % 4;
    } else if (ev == ButtonEvent::SHORT_PRESS) {
        switch (menuIndex) {
            case 0:
                editingColor = 'G';
                _goTo(AppState::EDIT_THRESH);
                break;
            case 1:
            case 2:
                editingColor = 'R';
                _goTo(AppState::EDIT_THRESH);
                break;
            case 3:
                menuIndex = 2;
                _goTo(AppState::MAIN_MENU);
                break;
        }
    }
}

// ==========================================
// EDIT_THRESH
// ==========================================
void StateMachine::_handleEditThresh(ButtonEvent ev) {
    if (ev == ButtonEvent::ROTATE_CW || ev == ButtonEvent::ROTATE_CCW) {
        requestSound(SoundEvent::SCROLL);
        float step = (ev == ButtonEvent::ROTATE_CW) ? 0.5f : -0.5f;

        if (editingColor == 'G') {
            NVSManager::thresh.green += step;
            NVSManager::thresh.green  = constrain(
                NVSManager::thresh.green, 0.0f,
                NVSManager::thresh.red - 0.5f
            );
        } else {
            NVSManager::thresh.red += step;
            NVSManager::thresh.red  = constrain(
                NVSManager::thresh.red,
                NVSManager::thresh.green + 0.5f, 100.0f
            );
        }
    } else if (ev == ButtonEvent::SHORT_PRESS) {
        NVSManager::saveThresh();
        _goTo(AppState::THRESH_MENU);
    }
}

// ==========================================
// CAL_DI
// ==========================================
void StateMachine::_handleCalDI(ButtonEvent ev, const SensorData& sensor) {
    if (ev == ButtonEvent::LONG_PRESS) {
        tmp_v_di = sensor.currentVolt;
        _goTo(AppState::CAL_SALT);
    } else if (ev == ButtonEvent::SHORT_PRESS) {
        _lastCalState = AppState::CAL_DI;
        cancelSelect  = 1;
        _goTo(AppState::CAL_CANCEL_CONFIRM);
    }
}

// ==========================================
// CAL_SALT
// ==========================================
void StateMachine::_handleCalSalt(ButtonEvent ev, const SensorData& sensor) {
    if (ev == ButtonEvent::LONG_PRESS) {
        tmp_v_salt = sensor.currentVolt;
        tmp_t_salt = sensor.currentTemp;
        _goTo(AppState::CAL_FINISH);
    } else if (ev == ButtonEvent::SHORT_PRESS) {
        _lastCalState = AppState::CAL_SALT;
        cancelSelect  = 1;
        _goTo(AppState::CAL_CANCEL_CONFIRM);
    }
}

// ==========================================
// CAL_FINISH — บันทึกค่าจริง
// ==========================================
void StateMachine::_handleCalFinish() {
    NVSManager::calib.v_di   = tmp_v_di;
    NVSManager::calib.v_salt = tmp_v_salt;
    NVSManager::calib.t_salt = tmp_t_salt;
    NVSManager::saveCalib();
    requestSound(SoundEvent::SUCCESS);
    _goTo(AppState::MAIN_SCREEN);
}

// ==========================================
// CAL_CANCEL_CONFIRM
// ==========================================
void StateMachine::_handleCalCancelConfirm(ButtonEvent ev) {
    if (ev == ButtonEvent::ROTATE_CW || ev == ButtonEvent::ROTATE_CCW) {
        cancelSelect = (cancelSelect == 0) ? 1 : 0;
    } else if (ev == ButtonEvent::SHORT_PRESS) {
        if (cancelSelect == 0) {
            // YES → ยกเลิก calibration กลับ main menu
            menuIndex = 3;
            _goTo(AppState::MAIN_MENU);
        } else {
            // NO → กลับไปทำ calibration ต่อ
            _goTo(_lastCalState);
        }
    }
}