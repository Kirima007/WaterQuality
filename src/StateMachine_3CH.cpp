#include "StateMachine.h"
#include "SensorMath.h"
#include "SimTask.h"

#if SENSOR_COUNT == 3
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
    _goTo(AppState::MAIN_MENU);
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
    if (next == AppState::SIM_SENDING) {
        _returnState = _current;
    }
    _prev    = _current;
    _current = next;
    _changed = true;
}

// ==========================================
// handleEvent — จุดเข้าหลัก
// ==========================================
void StateMachine::handleEvent(ButtonEvent ev) {
    SensorData sensor{};
    xQueuePeek(sensorQueue, &sensor, 0);

    switch (_current) {
        case AppState::MAIN_SCREEN:         _handleMainScreen(ev);              break;
        case AppState::MAIN_MENU:           _handleMainMenu(ev);                break;
        case AppState::MONITOR_MENU:        _handleMonitorMenu(ev);             break;
        case AppState::READ_GPS:            _handleReadGPS(ev);                 break;
        case AppState::THRESH_PARAM_MENU:   _handleThreshParamMenu(ev);         break;
        case AppState::THRESH_MENU:         _handleThreshMenu(ev);              break;
        case AppState::EDIT_THRESH:         _handleEditThresh(ev);              break;
        case AppState::CAL_PARAM_MENU:      _handleCalParamMenu(ev);            break;
        case AppState::CAL_MENU:            _handleCalMenu(ev);                 break;
        case AppState::SYSTEM_INFO:         _handleSystemInfo(ev);              break;
        case AppState::CAL_MANUAL:          _handleCalManual(ev, sensor);       break;
        case AppState::EDIT_CAL_MANUAL:     _handleEditCalManual(ev);           break;
        case AppState::CAL_DI:              _handleCalDI(ev, sensor);           break;
        case AppState::CAL_SALT:            _handleCalSalt(ev, sensor);         break;
        case AppState::CAL_FINISH:          _handleCalFinish();                 break;
        case AppState::CAL_CANCEL_CONFIRM:  _handleCalCancelConfirm(ev);        break;
        case AppState::SIM_SENDING:         _handleSimSending(ev);              break;
        case AppState::SIM_RESULT:          _handleSimResult(ev);               break;
        case AppState::NETWORK_STATUS:      _handleNetworkStatus(ev);           break;
        case AppState::SYSTEM_SETUP:        _handleSystemSetup(ev);             break;
        default: break;
    }
}

// ==========================================
// MAIN_SCREEN (Displaying one sensor)
// ==========================================
void StateMachine::_handleMainScreen(ButtonEvent ev) {
    if (ev == ButtonEvent::SHORT_PRESS) {
        menuIndex = currentParam; // กลับไปเลือกเซ็นเซอร์เดิม
        requestSound(SoundEvent::BACK);
        _goTo(AppState::MONITOR_MENU);
    }
    else if (ev == ButtonEvent::LONG_PRESS) {
        if (NVSManager::config.networkMode == NET_MODE_SIM) {
            SimTask::requestSend(currentParam);
        } else {
            WifiTask::requestSend(currentParam);
        }
        requestSound(SoundEvent::SELECT);
        _goTo(AppState::SIM_SENDING);
    }
}

void StateMachine::_handleMainMenu(ButtonEvent ev) {
    if (ev == ButtonEvent::ROTATE_CW) {
        menuIndex = (menuIndex + 1) % 7; 
        requestSound(SoundEvent::SCROLL);
    } else if (ev == ButtonEvent::ROTATE_CCW) {
        menuIndex = (menuIndex - 1 + 7) % 7;
        requestSound(SoundEvent::SCROLL);
    } else if (ev == ButtonEvent::SHORT_PRESS) {
        switch (menuIndex) {
            case 0: 
                menuIndex = 0;
                _goTo(AppState::MONITOR_MENU);        requestSound(SoundEvent::SELECT); break;
            case 1: 
                _goTo(AppState::NETWORK_STATUS);      requestSound(SoundEvent::SELECT); break;
            case 2: 
                menuIndex = 0;
            _goTo(AppState::SYSTEM_SETUP);        requestSound(SoundEvent::SELECT); break;
            case 3: _goTo(AppState::READ_GPS);        requestSound(SoundEvent::SELECT); break;
            case 4: 
                menuIndex = 0;
                _goTo(AppState::THRESH_PARAM_MENU);   requestSound(SoundEvent::SELECT); break;
            case 5: 
                menuIndex = 0;
                _goTo(AppState::CAL_PARAM_MENU);      requestSound(SoundEvent::SELECT); break;
        case 6: _goTo(AppState::SYSTEM_INFO);     requestSound(SoundEvent::SELECT); break;
        }
    }
}

// ==========================================
// MONITOR_MENU (Select Sensor)
// ==========================================
void StateMachine::_handleMonitorMenu(ButtonEvent ev) {
    if (ev == ButtonEvent::ROTATE_CW) {
        menuIndex = (menuIndex + 1) % 4;
        requestSound(SoundEvent::SCROLL);
    } else if (ev == ButtonEvent::ROTATE_CCW) {
        menuIndex = (menuIndex - 1 + 4) % 4;
        requestSound(SoundEvent::SCROLL);
    } else if (ev == ButtonEvent::SHORT_PRESS) {
        if (menuIndex < 3) {
            currentParam = menuIndex; // 0=EC, 1=pH, 2=DO
            _goTo(AppState::MAIN_SCREEN);
            requestSound(SoundEvent::SELECT);
        } else {
            menuIndex = 0;
            _goTo(AppState::MAIN_MENU);
            requestSound(SoundEvent::BACK);
        }
    }
}

// ==========================================
// READ_GPS
// ==========================================
void StateMachine::_handleReadGPS(ButtonEvent ev) {
    if (ev == ButtonEvent::SHORT_PRESS) {
        requestSound(SoundEvent::BACK);
        menuIndex = 3;
        _goTo(AppState::MAIN_MENU);
    }
}

// ==========================================
// THRESH_PARAM_MENU
// ==========================================
void StateMachine::_handleThreshParamMenu(ButtonEvent ev) {
    if (ev == ButtonEvent::ROTATE_CW) {
        menuIndex = (menuIndex + 1) % 4;
        requestSound(SoundEvent::SCROLL);
    } else if (ev == ButtonEvent::ROTATE_CCW) {
        menuIndex = (menuIndex - 1 + 4) % 4;
        requestSound(SoundEvent::SCROLL);
    } else if (ev == ButtonEvent::SHORT_PRESS) {
        if (menuIndex < 3) {
            currentParam = menuIndex;
            menuIndex = 0;
            _goTo(AppState::THRESH_MENU);
            requestSound(SoundEvent::SELECT);
        } else {
            menuIndex = 4;
            _goTo(AppState::MAIN_MENU);
            requestSound(SoundEvent::BACK);
        }
    }
}

// ==========================================
// THRESH_MENU
// ==========================================
void StateMachine::_handleThreshMenu(ButtonEvent ev) {
    if (ev == ButtonEvent::ROTATE_CW) {
        requestSound(SoundEvent::SCROLL);
        menuIndex = (menuIndex + 1) % 4; // กลับมามี 4 เมนู (G, Y, R, Back)
    } else if (ev == ButtonEvent::ROTATE_CCW) {
        requestSound(SoundEvent::SCROLL);
        menuIndex = (menuIndex - 1 + 4) % 4;
    } else if (ev == ButtonEvent::SHORT_PRESS) {
        switch (menuIndex) {
            case 0: editingColor = 'G'; _goTo(AppState::EDIT_THRESH); requestSound(SoundEvent::SELECT); break;
            case 1: editingColor = 'Y'; _goTo(AppState::EDIT_THRESH); requestSound(SoundEvent::SELECT); break;
            case 2: editingColor = 'R'; _goTo(AppState::EDIT_THRESH); requestSound(SoundEvent::SELECT); break;
            case 3: 
                menuIndex = currentParam; // ชี้ที่เมนู LED Threshold ใน Main Menu
                _goTo(AppState::THRESH_PARAM_MENU);
                requestSound(SoundEvent::BACK);
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
        
        ThreshData* t = nullptr;
        if (currentParam == 0) t = &NVSManager::threshEC;
        else if (currentParam == 1) t = &NVSManager::threshPH;
        else t = &NVSManager::threshDO;

        if (editingColor == 'G') {
            t->green += step;
            t->green = constrain(t->green, 0.0f, t->yellow - 0.5f);
        } 
        else if (editingColor == 'Y') {
            t->yellow += step;
            t->yellow = constrain(t->yellow, t->green + 0.5f, t->red - 0.5f);
        }
        else {
            t->red += step;
            t->red = constrain(t->red, t->yellow + 0.5f, 100.0f);
        }
    } else if (ev == ButtonEvent::SHORT_PRESS) {
        requestSound(SoundEvent::BACK);
        NVSManager::saveThresh();
        // เซฟเสร็จแล้วให้กลับไปหน้าเลือกสี (Thresh Menu) เหมือนเดิม
        _goTo(AppState::THRESH_MENU);
    }
}

// ==========================================
// CAL_PARAM_MENU
// ==========================================
void StateMachine::_handleCalParamMenu(ButtonEvent ev) {
    if (ev == ButtonEvent::ROTATE_CW) {
        menuIndex = (menuIndex + 1) % 4;
        requestSound(SoundEvent::SCROLL);
    } else if (ev == ButtonEvent::ROTATE_CCW) {
        menuIndex = (menuIndex - 1 + 4) % 4;
        requestSound(SoundEvent::SCROLL);
    } else if (ev == ButtonEvent::SHORT_PRESS) {
        if (menuIndex < 3) {
            currentParam = menuIndex;
            menuIndex = 0;
            _goTo(AppState::CAL_MENU);
            requestSound(SoundEvent::SELECT);
        } else {
            menuIndex = 5;
            _goTo(AppState::MAIN_MENU);
            requestSound(SoundEvent::BACK);
        }
    }
}

void StateMachine::_handleCalMenu(ButtonEvent ev) {
    if (ev == ButtonEvent::ROTATE_CW) {
        requestSound(SoundEvent::SCROLL);
        menuIndex = (menuIndex + 1) % 4;
    } else if (ev == ButtonEvent::ROTATE_CCW) {
        requestSound(SoundEvent::SCROLL);
        menuIndex = (menuIndex - 1 + 4) % 4;
    } else if (ev == ButtonEvent::SHORT_PRESS) {
        switch (menuIndex) {
            case 0: requestSound(SoundEvent::SELECT); _goTo(AppState::CAL_DI); break;
            case 1: requestSound(SoundEvent::SELECT); menuIndex = 0; _goTo(AppState::CAL_MANUAL); break;
            case 2: 
                requestSound(SoundEvent::SELECT);
                if (NVSManager::config.networkMode == NET_MODE_SIM) SimTask::requestSendCalib(currentParam);
                else WifiTask::requestSendCalib(currentParam);
                _goTo(AppState::SIM_SENDING);
                break;
            case 3: 
                menuIndex = currentParam;
                _goTo(AppState::CAL_PARAM_MENU); 
                requestSound(SoundEvent::BACK); 
                break;
        }
    }
}

void StateMachine::_handleSystemInfo(ButtonEvent ev) {
    if (ev == ButtonEvent::SUPER_LONG_PRESS) {
        requestSound(SoundEvent::SUCCESS); // หรือเสียงที่บ่งบอกว่าสำเร็จ
        NVSManager::reset();
        delay(500); // รอให้เสียงเล่นและบันทึกเสร็จ
        ESP.restart();
    } else if (ev == ButtonEvent::SHORT_PRESS) {
        requestSound(SoundEvent::BACK);
        menuIndex = 6;
        _goTo(AppState::MAIN_MENU);
    }
}

void StateMachine::_handleCalManual(ButtonEvent ev, const SensorData& sensor) {
    if (ev == ButtonEvent::ROTATE_CW) {
        requestSound(SoundEvent::SCROLL);
        menuIndex = (menuIndex + 1) % 4;
    } else if (ev == ButtonEvent::ROTATE_CCW) {
        requestSound(SoundEvent::SCROLL);
        menuIndex = (menuIndex - 1 + 4) % 4;
    } else if (ev == ButtonEvent::SHORT_PRESS) {
        switch (menuIndex) {
            case 0: editingColor = 'A'; _goTo(AppState::EDIT_CAL_MANUAL); requestSound(SoundEvent::SELECT); break;
            case 1: editingColor = 'B'; _goTo(AppState::EDIT_CAL_MANUAL); requestSound(SoundEvent::SELECT); break;
            case 2: 
                NVSManager::saveCalib(); // บันทึกลง NVS เมื่อกดปุ่ม Save And Send เท่านั้น
                if (NVSManager::config.networkMode == NET_MODE_SIM) SimTask::requestSendCalib(currentParam);
                else WifiTask::requestSendCalib(currentParam);
                _goTo(AppState::SIM_SENDING);
                _returnState = AppState::MAIN_MENU; // บังคับให้กลับหน้า Menu หลังส่ง Calib เสร็จ
                requestSound(SoundEvent::BACK);
                break;
            case 3:
                NVSManager::load(); // หากกด Back ให้โหลดค่าเดิมกลับมาทับค่าใหม่ที่ยังไม่ได้บันทึก
                menuIndex = 1;
                _goTo(AppState::CAL_MENU);
                requestSound(SoundEvent::BACK);
                break;
        }
    }
}

void StateMachine::_handleEditCalManual(ButtonEvent ev) {
    if (ev == ButtonEvent::ROTATE_CW || ev == ButtonEvent::ROTATE_CCW) {
        requestSound(SoundEvent::SCROLL);
        float step = (ev == ButtonEvent::ROTATE_CW) ? 0.001f : -0.001f;
        CalibData* c = nullptr;
        if (currentParam == 0) c = &NVSManager::calibEC;
        else if (currentParam == 1) c = &NVSManager::calibPH;
        else c = &NVSManager::calibDO;

        if (editingColor == 'A') {
            c->alpha += step;
            if (c->alpha < 0.001f) c->alpha = 0.001f;
        } else {
            c->beta += step;
        }
    } 
    else if (ev == ButtonEvent::SHORT_PRESS) {
        requestSound(SoundEvent::BACK);
        _goTo(AppState::CAL_MANUAL);
    }
}

void StateMachine::_handleCalDI(ButtonEvent ev, const SensorData& sensor) {
    if (ev == ButtonEvent::LONG_PRESS) {
        if (SensorMath::captureStableValue(currentParam, tmp_v_di, tmp_t_di)) {
            requestSound(SoundEvent::SUCCESS);
            _goTo(AppState::CAL_SALT);
        } else {
            requestSound(SoundEvent::BACK); 
        }
    } else if (ev == ButtonEvent::SHORT_PRESS) {
        _lastCalState = AppState::CAL_DI;
        cancelSelect  = 1;
        requestSound(SoundEvent::SCROLL);
        _goTo(AppState::CAL_CANCEL_CONFIRM);
    }
}

void StateMachine::_handleCalSalt(ButtonEvent ev, const SensorData& sensor) {
    if (ev == ButtonEvent::LONG_PRESS) {
        if (SensorMath::captureStableValue(currentParam, tmp_v_salt, tmp_t_salt)) {
            requestSound(SoundEvent::SUCCESS);
            _goTo(AppState::CAL_FINISH);
        } else {
            requestSound(SoundEvent::BACK); 
        }
    } else if (ev == ButtonEvent::SHORT_PRESS) {
        _lastCalState = AppState::CAL_SALT;
        cancelSelect  = 1;
        requestSound(SoundEvent::SCROLL);
        _goTo(AppState::CAL_CANCEL_CONFIRM);
    }
}

void StateMachine::_handleCalFinish() {
    float newAlpha, newBeta;
    bool isSuccess = false;
    
    if (SensorMath::computeAlphaBeta(currentParam, tmp_v_di, tmp_t_di, tmp_v_salt, tmp_t_salt, newAlpha, newBeta)) {
        if (currentParam == 0) { NVSManager::calibEC.alpha = newAlpha; NVSManager::calibEC.beta = newBeta; }
        else if (currentParam == 1) { NVSManager::calibPH.alpha = newAlpha; NVSManager::calibPH.beta = newBeta; }
        else { NVSManager::calibDO.alpha = newAlpha; NVSManager::calibDO.beta = newBeta; }
        NVSManager::saveCalib();
        requestSound(SoundEvent::SUCCESS);
        isSuccess = true;
    } else {
        requestSound(SoundEvent::BACK);
    }
    
    if (isSuccess) {
        if (NVSManager::config.networkMode == NET_MODE_SIM) SimTask::requestSendCalib(currentParam);
        else WifiTask::requestSendCalib(currentParam);
        _goTo(AppState::SIM_SENDING); 
        _returnState = AppState::MAIN_MENU; // บังคับให้กลับหน้า Menu หลังส่ง Calib เสร็จ
    } else {
        menuIndex = 0;
        _goTo(AppState::CAL_MENU);
    }
}

void StateMachine::_handleCalCancelConfirm(ButtonEvent ev) {
    if (ev == ButtonEvent::ROTATE_CW || ev == ButtonEvent::ROTATE_CCW) {
        cancelSelect = (cancelSelect == 0) ? 1 : 0;
        requestSound(SoundEvent::SCROLL);
    } else if (ev == ButtonEvent::SHORT_PRESS) {
        if (cancelSelect == 0) {
            requestSound(SoundEvent::SELECT);
            menuIndex = 0;
            _goTo(AppState::CAL_MENU);
        } else {
            requestSound(SoundEvent::BACK);
            _goTo(_lastCalState);
        }
    }
}

void StateMachine::_handleSimSending(ButtonEvent ev) {
    if (ev == ButtonEvent::SHORT_PRESS || ev == ButtonEvent::LONG_PRESS) {
        requestSound(SoundEvent::BACK);
        _goTo(_returnState);
    }
}

void StateMachine::_handleSimResult(ButtonEvent ev) {
    if (ev == ButtonEvent::SHORT_PRESS || ev == ButtonEvent::LONG_PRESS) {
        requestSound(SoundEvent::BACK);
        _goTo(_returnState);
    }
}

void StateMachine::onSimSendComplete(bool success, int httpCode) {
    simLastSuccess  = success;
    simLastHttpCode = httpCode;
    if (success) requestSound(SoundEvent::SUCCESS);
    else         requestSound(SoundEvent::BACK);
    _goTo(AppState::SIM_RESULT);
}

void StateMachine::_handleSystemSetup(ButtonEvent ev) {
    if (ev == ButtonEvent::ROTATE_CW || ev == ButtonEvent::ROTATE_CCW) {
        requestSound(SoundEvent::SCROLL);
        menuIndex = (ev == ButtonEvent::ROTATE_CW) ? (menuIndex + 1) % 3 : (menuIndex - 1 + 3) % 3;
    } else if (ev == ButtonEvent::SHORT_PRESS) {
        if (menuIndex == 0) {
            NVSManager::config.isMuted = !NVSManager::config.isMuted;
            requestSound(SoundEvent::SELECT); 
            NVSManager::saveConfig();
        } else if (menuIndex == 1) {
            if (NVSManager::config.networkMode == NET_MODE_SIM) NVSManager::config.networkMode = NET_MODE_WIFI;
            else NVSManager::config.networkMode = NET_MODE_SIM;
            requestSound(SoundEvent::SELECT);
            NVSManager::saveConfig();
        } else if (menuIndex == 2) {
            requestSound(SoundEvent::BACK);
            menuIndex = 2; // กลับไปชี้ที่เมนู System Setup ในหน้าหลัก
            _goTo(AppState::MAIN_MENU);
        }
    }
}

void StateMachine::_handleNetworkStatus(ButtonEvent ev) {
    if (ev == ButtonEvent::SHORT_PRESS || ev == ButtonEvent::LONG_PRESS) {
        requestSound(SoundEvent::BACK);
        menuIndex = 1;
        _goTo(AppState::MAIN_MENU);
    }
}

#endif