#include "StateMachine.h"
#include "SensorMath.h"
#include "SimTask.h"
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
        case AppState::CAL_MENU:            _handleCalMenu(ev);                 break;
        case AppState::SYSTEM_INFO:         _handleSystemInfo(ev);              break;
        case AppState::CAL_MANUAL:          _handleCalManual(ev, sensor);       break;
        case AppState::EDIT_CAL_MANUAL:     _handleEditCalManual(ev);           break;
        case AppState::CAL_DI:              _handleCalDI(ev, sensor);           break;
        case AppState::CAL_SALT:            _handleCalSalt(ev, sensor);         break;
        case AppState::CAL_FINISH:          _handleCalFinish();                 break;
        case AppState::CAL_CANCEL_CONFIRM:  _handleCalCancelConfirm(ev);        break;
        // case AppState::SIM_STATUS:          _handleSimStatus(ev);               break;
        case AppState::SIM_SENDING:         _handleSimSending(ev);              break;
        case AppState::SIM_RESULT:          _handleSimResult(ev);               break;
        case AppState::NETWORK_MENU:        _handleNetworkMenu(ev);             break;
        case AppState::NETWORK_STATUS:      _handleNetworkStatus(ev);           break;
        case AppState::BUZZER_MENU:          _handleBuzzerMenu(ev);             break;
        default: break;
    }
}

// ==========================================
// MAIN_SCREEN
// ==========================================
void StateMachine::_handleMainScreen(ButtonEvent ev) {
    if (ev == ButtonEvent::SHORT_PRESS) {
        menuIndex = 0;
        requestSound(SoundEvent::SELECT);
        _goTo(AppState::MAIN_MENU);
    }
    else if (ev == ButtonEvent::LONG_PRESS) {
        if (NVSManager::config.networkMode == NET_MODE_SIM) {
            SimTask::requestSend();
        } else {
            WifiTask::requestSend();
        }
        requestSound(SoundEvent::SELECT);
        _goTo(AppState::SIM_SENDING); // (ใช้หน้าจอเดิมได้เลย ประหยัดแรง!)
    }
}

// ==========================================
// MAIN_MENU
// ==========================================
void StateMachine::_handleMainMenu(ButtonEvent ev) {
    if (ev == ButtonEvent::ROTATE_CW) {
        menuIndex = (menuIndex + 1) % 9;
        requestSound(SoundEvent::SCROLL);
    } else if (ev == ButtonEvent::ROTATE_CCW) {
        menuIndex = (menuIndex - 1 + 9) % 9;
        requestSound(SoundEvent::SCROLL);
    } else if (ev == ButtonEvent::SHORT_PRESS) {
        switch (menuIndex) {
            case 0: _goTo(AppState::READ_TEMP);       requestSound(SoundEvent::SELECT); break;
            case 1: 
                _goTo(AppState::NETWORK_STATUS);      requestSound(SoundEvent::SELECT); break;
            case 2: 
                menuIndex = 0;
                _goTo(AppState::NETWORK_MENU);        requestSound(SoundEvent::SELECT); break;
            case 3: _goTo(AppState::READ_GPS);        requestSound(SoundEvent::SELECT); break;
            case 4: 
                menuIndex = 0;
                _goTo(AppState::THRESH_MENU);         requestSound(SoundEvent::SELECT); break;
            case 5: 
                menuIndex = 0;
                _goTo(AppState::CAL_MENU);            requestSound(SoundEvent::SELECT); break;
            case 6: _goTo(AppState::BUZZER_MENU);     requestSound(SoundEvent::SELECT); break;
            case 7: _goTo(AppState::SYSTEM_INFO);     requestSound(SoundEvent::SELECT); break;
            case 8: _goTo(AppState::MAIN_SCREEN);     requestSound(SoundEvent::BACK);   break;
        }
    }
}

// ==========================================
// READ_TEMP / READ_GPS
// ==========================================
void StateMachine::_handleReadTemp(ButtonEvent ev) {
    if (ev == ButtonEvent::SHORT_PRESS) {
        requestSound(SoundEvent::BACK);
        menuIndex = 0;
        _goTo(AppState::MAIN_MENU);
    }
}

void StateMachine::_handleReadGPS(ButtonEvent ev) {
    if (ev == ButtonEvent::SHORT_PRESS) {
        requestSound(SoundEvent::BACK);
        menuIndex = 3;
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
                requestSound(SoundEvent::SELECT);
                break;
            case 1:
                editingColor = 'Y';
                _goTo(AppState::EDIT_THRESH);
                requestSound(SoundEvent::SELECT);
                break;
            case 2:
                editingColor = 'R';
                _goTo(AppState::EDIT_THRESH);
                requestSound(SoundEvent::SELECT);
                break;
            case 3:
                menuIndex = 4;
                _goTo(AppState::MAIN_MENU);
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

        if (editingColor == 'G') {
            NVSManager::thresh.green += step;
            NVSManager::thresh.green  = constrain(
                NVSManager::thresh.green, 
                0.0f, 
                NVSManager::thresh.yellow - 0.5f
            );
        } 
        else if (editingColor == 'Y') {
            NVSManager::thresh.yellow += step;
            NVSManager::thresh.yellow  = constrain(
                NVSManager::thresh.yellow,
                NVSManager::thresh.green + 0.5f,
                NVSManager::thresh.red - 0.5f
            );
        }
        else { // กรณีแก้สีแดง ('R')
            NVSManager::thresh.red += step;
            NVSManager::thresh.red  = constrain(
                NVSManager::thresh.red,
                NVSManager::thresh.yellow + 0.5f,
                100.0f
            );
        }
    } else if (ev == ButtonEvent::SHORT_PRESS) {
        requestSound(SoundEvent::BACK);
        NVSManager::saveThresh();
        _goTo(AppState::THRESH_MENU);
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
            case 0:  // Auto Calibrate
                requestSound(SoundEvent::SELECT);
                _goTo(AppState::CAL_DI);
                break;
            case 1:  // Manual Calibrate
                requestSound(SoundEvent::SELECT);
                menuIndex = 0;
                _goTo(AppState::CAL_MANUAL);
                break;
            case 2:  // Send Calib ← ใหม่
                requestSound(SoundEvent::SELECT);
                if (NVSManager::config.networkMode == NET_MODE_SIM) {
                    SimTask::requestSendCalib();
                } else {
                    WifiTask::requestSendCalib();
                }
                _goTo(AppState::SIM_SENDING);
                break;
            case 3:  // Back
                requestSound(SoundEvent::BACK);
                menuIndex = 5;
                _goTo(AppState::MAIN_MENU);
                break;
        }
    }
}


void StateMachine::_handleSystemInfo(ButtonEvent ev) {
    if (ev == ButtonEvent::SHORT_PRESS || ev == ButtonEvent::LONG_PRESS) {
        requestSound(SoundEvent::BACK);
        menuIndex = 7;
        _goTo(AppState::MAIN_MENU);
    }
}


void StateMachine::_handleCalManual(ButtonEvent ev, const SensorData& sensor) {
    if (ev == ButtonEvent::ROTATE_CW) {
        requestSound(SoundEvent::SCROLL);
        menuIndex = (menuIndex + 1) % 3; // 0=A, 1=B, 2=Save, 3=Back
    } else if (ev == ButtonEvent::ROTATE_CCW) {
        requestSound(SoundEvent::SCROLL);
        menuIndex = (menuIndex - 1 + 3) % 3;
    } else if (ev == ButtonEvent::SHORT_PRESS) {
        switch (menuIndex) {
            case 0: // เลือกปรับค่า A
                editingColor = 'A'; // ยืมใช้ตัวแปรเดิมจำสถานะว่ากำลังแก้ A
                _goTo(AppState::EDIT_CAL_MANUAL);
                requestSound(SoundEvent::SELECT);
                break;
            case 1: // เลือกปรับค่า B
                editingColor = 'B'; // จำสถานะว่ากำลังแก้ B
                _goTo(AppState::EDIT_CAL_MANUAL);
                requestSound(SoundEvent::SELECT);
                break;
            case 2: // Back
                menuIndex = 0; // กลับไปชี้ที่เมนู "Manual Calibrate" 
                if (NVSManager::config.networkMode == NET_MODE_SIM) {
                    SimTask::requestSendCalib();
                } else {
                    WifiTask::requestSendCalib();
                }
                _goTo(AppState::SIM_SENDING);
                requestSound(SoundEvent::BACK);
                break;
        }
    }
}

void StateMachine::_handleEditCalManual(ButtonEvent ev) {
    if (ev == ButtonEvent::ROTATE_CW || ev == ButtonEvent::ROTATE_CCW) {
        requestSound(SoundEvent::SCROLL);
        
        // ความละเอียดทีละ 0.001 (แบบ Manual Calibrate ของรุ่นพี่)
        float step = (ev == ButtonEvent::ROTATE_CW) ? 0.001f : -0.001f;

        if (editingColor == 'A') {
            NVSManager::calib.alpha += step;
            // ป้องกัน Alpha ติดลบหรือเป็นศูนย์ (เดี๋ยวเอาไปคูณแล้วค่าน้ำหายหมด)
            if (NVSManager::calib.alpha < 0.001f) {
                NVSManager::calib.alpha = 0.001f;
            }
        } else if (editingColor == 'B') {
            NVSManager::calib.beta += step;
        }
    } 
    else if (ev == ButtonEvent::SHORT_PRESS) {
        // กดยืนยันการแก้ไข (แต่ยังไม่เซฟลง NVS) ให้กลับมาหน้าเลือกเมนู
        requestSound(SoundEvent::BACK);
        
        if (editingColor == 'A') {
            // ถ้าเพิ่งแก้ Alpha เสร็จ ให้ไปชี้ที่ Beta ต่อ
            menuIndex = 0; // ชี้ที่ "Beta"
            NVSManager::saveCalib(); // เซฟค่าใหม่ลง NVS ทันที (เผื่อมีปัญหาแล้วต้องยกเลิกกลางคัน จะได้ไม่เสียของเก่า)
            _goTo(AppState::CAL_MANUAL);
        } else {            // ถ้าเพิ่งแก้ Beta เสร็จ ให้ไปชี้ที่ Save
            menuIndex = 1; // ชี้ที่ "Save"
            NVSManager::saveCalib(); // เซฟค่าใหม่ลง NVS ทันที (เผื่อมีปัญหาแล้วต้องยกเลิกกลางคัน จะได้ไม่เสียของเก่า)
            _goTo(AppState::CAL_MANUAL);
        }
    }
}

// ==========================================
// CAL_DI
// ==========================================

bool StateMachine::_captureStableValue(float &capturedVolt, float &capturedTemp) {
    const int N = 5;
    float bufVolt[N];
    float sumVolt = 0.0f, sumTemp = 0.0f;
    
    for (int i = 0; i < N; i++) {
        SensorData s;
        xQueuePeek(sensorQueue, &s, 0); // ดึงค่าล่าสุดจาก SensorTask
        bufVolt[i] = s.currentVolt;
        sumVolt += s.currentVolt;
        sumTemp += s.currentTemp;
        
        vTaskDelay(pdMS_TO_TICKS(500)); // หน่วงเวลาดึงค่าทีละ 0.5 วินาที
    }
    
    float minV = bufVolt[0], maxV = bufVolt[0];
    for (int i = 0; i < N; i++) {
        if (bufVolt[i] < minV) minV = bufVolt[i];
        if (bufVolt[i] > maxV) maxV = bufVolt[i];
    }
    
    // ถ้าระหว่าง 2.5 วินาทีที่ผ่านมา แรงดันแกว่งเกิน 0.05V ถือว่าน้ำยังไม่นิ่ง!
    if ((maxV - minV) > 0.05f) {
        return false; 
    }
    
    capturedVolt = sumVolt / N;
    capturedTemp = sumTemp / N;
    return true;
}
void StateMachine::_handleCalDI(ButtonEvent ev, const SensorData& sensor) {
    if (ev == ButtonEvent::LONG_PRESS) {
        // เมื่อกดค้าง ให้เริ่มเช็คความนิ่ง
        if (_captureStableValue(tmp_v_di, tmp_t_di)) {
            requestSound(SoundEvent::SUCCESS);
            _goTo(AppState::CAL_SALT);
        } else {
            // ถ้าน้ำแกว่งไป ให้ร้องเตือน Error (ไม่ต้องเปลี่ยนหน้า)
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
        if (_captureStableValue(tmp_v_salt, tmp_t_salt)) {
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
    // คำนวณหา Alpha/Beta จากค่าน้ำ DI และ Salt ที่เพิ่งจดจำมา
    if (SensorMath::computeAlphaBeta(tmp_v_di, tmp_t_di, tmp_v_salt, tmp_t_salt, newAlpha, newBeta)) {
        
        // ถ้าคำนวณสำเร็จ ให้เซฟทุกอย่างลง NVS (หรือ EEPROM ของคุณ)
        NVSManager::calib.v_di   = tmp_v_di;
        NVSManager::calib.v_salt = tmp_v_salt;
        NVSManager::calib.t_salt = tmp_t_salt;
        NVSManager::calib.alpha  = newAlpha;
        NVSManager::calib.beta   = newBeta; 
        NVSManager::saveCalib();

        
        requestSound(SoundEvent::SUCCESS);
        isSuccess = true;
    } else {
        // ถ้าคำนวณพัง (เช่น จุ่มน้ำผิดขวด แรงดันเท่ากัน) ให้ร้องเตือน
        requestSound(SoundEvent::BACK);
    }
    
    if (isSuccess) {
        if (NVSManager::config.networkMode == NET_MODE_SIM) {
            SimTask::requestSendCalib();
        } else {
            WifiTask::requestSendCalib();
        }
        // เปลี่ยนหน้าจอไปหน้า ส่งข้อมูล
        _goTo(AppState::SIM_SENDING); 
    } else {
        // ถ้า Calibrate พัง (isSuccess เป็น false) ให้เด้งกลับไปหน้าเมนูหลักของ Calibrate แทน
        menuIndex = 0;
        _goTo(AppState::CAL_MENU);
    }
}

// ==========================================
// CAL_CANCEL_CONFIRM
// ==========================================
void StateMachine::_handleCalCancelConfirm(ButtonEvent ev) {
    if (ev == ButtonEvent::ROTATE_CW || ev == ButtonEvent::ROTATE_CCW) {
        cancelSelect = (cancelSelect == 0) ? 1 : 0;
        requestSound(SoundEvent::SCROLL);  // สลับ YES/NO
    } else if (ev == ButtonEvent::SHORT_PRESS) {
        if (cancelSelect == 0) {
            // YES → ยกเลิก calibration กลับ main menu
            requestSound(SoundEvent::SELECT);
            menuIndex = 0;
            _goTo(AppState::CAL_MENU);
        } else {
            // NO → กลับไปทำ calibration ต่อ
            requestSound(SoundEvent::BACK);
            _goTo(_lastCalState);
        }
    }
}

// ==========================================
// SIM_STATUS, SIM_SENDING, SIM_RESULT
// ==========================================
// void StateMachine::_handleSimStatus(ButtonEvent ev) {
//     if (ev == ButtonEvent::SHORT_PRESS) {
//         requestSound(SoundEvent::BACK);
//         _goTo(AppState::MAIN_SCREEN);

//     }
//     else if (ev == ButtonEvent::LONG_PRESS) {
//         SimTask::requestSend();
//         requestSound(SoundEvent::SELECT);
//         _goTo(AppState::SIM_SENDING);
//     }
// }
void StateMachine::_handleSimSending(ButtonEvent ev) {
    if (ev == ButtonEvent::SHORT_PRESS || ev == ButtonEvent::LONG_PRESS) {
        requestSound(SoundEvent::BACK);
        _goTo(AppState::MAIN_SCREEN);
    }
}

void StateMachine::_handleSimResult(ButtonEvent ev) {
    if (ev == ButtonEvent::SHORT_PRESS || ev == ButtonEvent::LONG_PRESS) {
        requestSound(SoundEvent::BACK);
        _goTo(AppState::MAIN_SCREEN);
    }
}

void StateMachine::onSimSendComplete(bool success, int httpCode) {
    simLastSuccess  = success;
    simLastHttpCode = httpCode;
    if (success) requestSound(SoundEvent::SUCCESS);
    else         requestSound(SoundEvent::BACK);
    _goTo(AppState::SIM_RESULT);
}



// ==========================================
// NETWORK_MENU
// ==========================================
void StateMachine::_handleNetworkMenu(ButtonEvent ev) {
    // ตอนผู้ใช้หมุนลูกบิด
    if (ev == ButtonEvent::ROTATE_CW || ev == ButtonEvent::ROTATE_CCW) {
        requestSound(SoundEvent::SCROLL);
        
        // สลับโหมด 0 (SIM) กับ 1 (WIFI)
        if (NVSManager::config.networkMode == NET_MODE_SIM) {
            NVSManager::config.networkMode = NET_MODE_WIFI;
        } else {
            NVSManager::config.networkMode = NET_MODE_SIM;
        }
        
    } 
    // ตอนผู้ใช้กดปุ่มเพื่อยืนยัน
    else if (ev == ButtonEvent::SHORT_PRESS) {
        requestSound(SoundEvent::BACK);
        
        NVSManager::saveConfig(); // สั่งเซฟค่าลง EEPROM ทันที!
        
        menuIndex = 2;
        _goTo(AppState::MAIN_MENU);
    }
}

// ==========================================
// NETWORK_STATUS
// ==========================================
void StateMachine::_handleNetworkStatus(ButtonEvent ev) {
    // หน้านี้แค่แสดงผล ถ้ากดปุ่มอะไรก็ตามให้เด้งกลับไปที่ Main Menu
    if (ev == ButtonEvent::SHORT_PRESS || ev == ButtonEvent::LONG_PRESS) {
        requestSound(SoundEvent::BACK);
        menuIndex = 1;
        _goTo(AppState::MAIN_MENU);
    }

    
}


// ==========================================
// BUZZER_MENU
// ==========================================
void StateMachine::_handleBuzzerMenu(ButtonEvent ev) {
    
    // 1. ถ้ามีการหมุนลูกบิดซ้ายหรือขวา
    if (ev == ButtonEvent::ROTATE_CW || ev == ButtonEvent::ROTATE_CCW) {
        // ให้สลับค่า Mute ปัจจุบัน (true เป็น false, false เป็น true)
        NVSManager::config.isMuted = !NVSManager::config.isMuted;
        requestSound(SoundEvent::SCROLL); 
    } 
    
    else if (ev == ButtonEvent::SHORT_PRESS) {
        requestSound(SoundEvent::BACK);
        
        NVSManager::saveConfig(); 
        
        menuIndex = 6; 
        
        // เปลี่ยน State กลับไปหน้า Main Menu
        _goTo(AppState::MAIN_MENU);
    }
}