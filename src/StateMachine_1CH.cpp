#include "StateMachine.h"
#include "SensorMath.h"
#include "SimTask.h"

#if SENSOR_COUNT == 1
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
        case AppState::TEMP_CAL:            _handleTempCal(ev);                 break;
        case AppState::SYSTEM_INFO:         _handleSystemInfo(ev);              break;
        case AppState::CAL_MANUAL:          _handleCalManual(ev, sensor);       break;
        case AppState::EDIT_CAL_MANUAL:     _handleEditCalManual(ev);           break;
        case AppState::CAL_DI:              _handleCalDI(ev, sensor);           break;
        case AppState::CAL_SALT:            _handleCalSalt(ev, sensor);         break;
        // CAL_FINISH ไม่รอรับ Event เพื่อให้มันข้ามไปส่งข้อมูลโดยอัตโนมัติ
        case AppState::CAL_CANCEL_CONFIRM:  _handleCalCancelConfirm(ev);        break;
        // case AppState::SIM_STATUS:          _handleSimStatus(ev);               break;
        case AppState::SIM_SENDING:         _handleSimSending(ev);              break;
        case AppState::SIM_RESULT:          _handleSimResult(ev);               break;
        case AppState::NETWORK_STATUS:      _handleNetworkStatus(ev);           break;
        case AppState::SYSTEM_SETUP:        _handleSystemSetup(ev);             break;
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
            SimTask::requestSend(0);
        } else {
            WifiTask::requestSend(0);
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
        menuIndex = (menuIndex + 1) % 8;
        requestSound(SoundEvent::SCROLL);
    } else if (ev == ButtonEvent::ROTATE_CCW) {
        menuIndex = (menuIndex - 1 + 8) % 8;
        requestSound(SoundEvent::SCROLL);
    } else if (ev == ButtonEvent::SHORT_PRESS) {
        switch (menuIndex) {
            case 0: _goTo(AppState::NETWORK_STATUS);  requestSound(SoundEvent::SELECT); break;
            case 1: _goTo(AppState::READ_GPS);        requestSound(SoundEvent::SELECT); break;
            case 2: 
                menuIndex = 0;
                _goTo(AppState::SYSTEM_SETUP);        requestSound(SoundEvent::SELECT); break;
            case 3: 
                menuIndex = 0;
                _goTo(AppState::CAL_MENU);            requestSound(SoundEvent::SELECT); break;
            case 4: _goTo(AppState::TEMP_CAL);        requestSound(SoundEvent::SELECT); break;
            case 5: 
                menuIndex = 0;
                _goTo(AppState::THRESH_MENU);         requestSound(SoundEvent::SELECT); break;
            case 6: _goTo(AppState::SYSTEM_INFO);     requestSound(SoundEvent::SELECT); break;
            case 7: _goTo(AppState::MAIN_SCREEN);     requestSound(SoundEvent::BACK);   break;
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
        menuIndex = (menuIndex + 1) % 4; // เหลือ 3 สี (G, Y, R) ไม่มี Back
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
                case 3: // Back
                menuIndex = 5; // ชี้ที่เมนู Alarm Limits
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
        // เซฟเสร็จแล้วให้กลับไปหน้าเมนูหลักเลย
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
                    SimTask::requestSendCalib(0);
                } else {
                    WifiTask::requestSendCalib(0);
                }
                _goTo(AppState::SIM_SENDING);
                break;
            case 3:  // Back
                requestSound(SoundEvent::BACK);
                menuIndex = 3;
                _goTo(AppState::MAIN_MENU);
                break;
        }
    }
}

void StateMachine::_handleTempCal(ButtonEvent ev) {
    if (ev == ButtonEvent::ROTATE_CW || ev == ButtonEvent::ROTATE_CCW) {
        requestSound(SoundEvent::SCROLL);
        float step = (ev == ButtonEvent::ROTATE_CW) ? 0.1f : -0.1f;
        NVSManager::tempOffset += step;
        NVSManager::tempOffset = constrain(NVSManager::tempOffset, -10.0f, 10.0f);
    } else if (ev == ButtonEvent::SHORT_PRESS) {
        requestSound(SoundEvent::BACK);
        NVSManager::saveCalib(); // บันทึก Offset ลง NVS
        menuIndex = 4;           // กลับไปชี้ที่เมนู Temp Calibrate
        _goTo(AppState::MAIN_MENU);
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
        menuIndex = (menuIndex + 1) % 4; // 0=A, 1=B, 2=Save, 3=Cancel
    } else if (ev == ButtonEvent::ROTATE_CCW) {
        requestSound(SoundEvent::SCROLL);
        menuIndex = (menuIndex - 1 + 4) % 4;
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
            case 2: // Save
                NVSManager::saveCalib(); // บันทึกลง NVS เมื่อกดปุ่ม Save And Send เท่านั้น
                if (NVSManager::config.networkMode == NET_MODE_SIM) {
                    SimTask::requestSendCalib(0);
                } else {
                    WifiTask::requestSendCalib(0);
                }
                _goTo(AppState::SIM_SENDING);
                _returnState = AppState::MAIN_MENU; // บังคับให้กลับหน้า Menu หลังส่ง Calib เสร็จ
                requestSound(SoundEvent::BACK);
                break;
            case 3: // Cancel
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
        
        // ความละเอียดทีละ 0.001 (แบบ Manual Calibrate ของรุ่นพี่)
        float step = (ev == ButtonEvent::ROTATE_CW) ? 0.001f : -0.001f;

        if (editingColor == 'A') {
            NVSManager::calibEC.alpha += step;
            // ป้องกัน Alpha ติดลบหรือเป็นศูนย์ (เดี๋ยวเอาไปคูณแล้วค่าน้ำหายหมด)
            if (NVSManager::calibEC.alpha < 0.001f) {
                NVSManager::calibEC.alpha = 0.001f;
            }
        } else if (editingColor == 'B') {
            NVSManager::calibEC.beta += step;
        }
    } 
    else if (ev == ButtonEvent::SHORT_PRESS) {
        // กดยืนยันการแก้ไข (แต่ยังไม่เซฟลง NVS) ให้กลับมาหน้าเลือกเมนู
        requestSound(SoundEvent::BACK);
        _goTo(AppState::CAL_MANUAL);
    }
}

// ==========================================
// CAL_DI
// ==========================================

void StateMachine::_handleCalDI(ButtonEvent ev, const SensorData& sensor) {
    if (ev == ButtonEvent::LONG_PRESS) {
        // เมื่อกดค้าง ให้เริ่มเช็คความนิ่ง
        if (SensorMath::captureStableValue(0, tmp_v_di, tmp_t_di)) {
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
        if (SensorMath::captureStableValue(0, tmp_v_salt, tmp_t_salt)) {
            
            float newAlpha, newBeta;
            // ทำการคำนวณทันที
            calibSuccess = SensorMath::computeAlphaBeta(0, tmp_v_di, tmp_t_di, tmp_v_salt, tmp_t_salt, newAlpha, newBeta);
            
            if (calibSuccess) {
                NVSManager::calibEC.alpha  = newAlpha;
                NVSManager::calibEC.beta   = newBeta; 
                NVSManager::saveCalib();
                requestSound(SoundEvent::SUCCESS);
            } else {
                requestSound(SoundEvent::BACK);
            }

            _goTo(AppState::CAL_FINISH);
            vTaskDelay(pdMS_TO_TICKS(1500)); // โชว์หน้าจอ 1.5 วินาที (ไม่ว่าจะ Success หรือ Fail)
            xQueueReset(inputQueue);

            if (calibSuccess) {
                if (NVSManager::config.networkMode == NET_MODE_SIM) SimTask::requestSendCalib(0);
                else WifiTask::requestSendCalib(0);
                _goTo(AppState::SIM_SENDING); 
                _returnState = AppState::MAIN_MENU; 
            } else {
                menuIndex = 0;
                _goTo(AppState::CAL_MENU); // ถ้าล้มเหลวก็เด้งกลับหน้าเมนูอัตโนมัติ
            }
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



// ==========================================
// NETWORK_MENU
// ==========================================
void StateMachine::_handleSystemSetup(ButtonEvent ev) {
    if (ev == ButtonEvent::ROTATE_CW || ev == ButtonEvent::ROTATE_CCW) {
        requestSound(SoundEvent::SCROLL);
        menuIndex = (ev == ButtonEvent::ROTATE_CW) ? (menuIndex + 1) % 3 : (menuIndex - 1 + 3) % 3;
    } 
    else if (ev == ButtonEvent::SHORT_PRESS) {
        if (menuIndex == 0) {
            if (NVSManager::config.networkMode == NET_MODE_SIM) NVSManager::config.networkMode = NET_MODE_WIFI;
            else NVSManager::config.networkMode = NET_MODE_SIM;
            requestSound(SoundEvent::SELECT);
            NVSManager::saveConfig();
        } else if (menuIndex == 1) {
            NVSManager::config.isMuted = !NVSManager::config.isMuted;
            requestSound(SoundEvent::SELECT); 
            NVSManager::saveConfig();
        } else if (menuIndex == 2) {
            requestSound(SoundEvent::BACK);
            menuIndex = 2; // กลับไปชี้ที่เมนู System Setup ในหน้าหลัก
            _goTo(AppState::MAIN_MENU);
        }
    }
}

// ==========================================
// NETWORK_STATUS
// ==========================================
void StateMachine::_handleNetworkStatus(ButtonEvent ev) {
    // หน้านี้แค่แสดงผล ถ้ากดปุ่มอะไรก็ตามให้เด้งกลับไปที่ Main Menu
    if (ev == ButtonEvent::SHORT_PRESS || ev == ButtonEvent::LONG_PRESS) {
        requestSound(SoundEvent::BACK);
        menuIndex = 0;
        _goTo(AppState::MAIN_MENU);
    }

    
}


#endif