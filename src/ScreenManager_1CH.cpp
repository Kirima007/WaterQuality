#include "ScreenManager.h"
#include "SimTask.h"

#if SENSOR_COUNT == 1

// ==========================================
// Constructor
// ==========================================
ScreenManager::ScreenManager(LiquidCrystal_I2C& lcd, StateMachine& sm)
    : _lcd(lcd), _sm(sm) {}

void ScreenManager::begin() {
    _lcd.init();
    _lcd.backlight();
    _lcd.clear();
}

// ==========================================
// FreeRTOS Task Entry Point
// ==========================================
void ScreenManager::taskEntry(void* param) {
    ScreenManager* self = static_cast<ScreenManager*>(param);
    AppState lastState  = AppState::STARTUP;

    for (;;) {
        // ดึงข้อมูลล่าสุดจาก Queue
        self->_updateData();

        // ถ้า State เปลี่ยน → clear LCD ก่อนวาดใหม่
        if (self->_sm.hasChanged()) {
            self->_lcd.clear();
        }

        // วาดหน้าจอตาม State
        switch (self->_sm.getState()) {
            case AppState::STARTUP:              self->_drawStartup();           break;
            case AppState::MAIN_SCREEN:          self->_drawMainScreen();        break;
            case AppState::MAIN_MENU:            self->_drawMainMenu();          break;
            case AppState::READ_TEMP:            self->_drawReadTemp();          break;
            case AppState::READ_GPS:             self->_drawReadGPS();           break;
            case AppState::THRESH_MENU:          self->_drawThreshMenu();        break;
            case AppState::EDIT_THRESH:          self->_drawEditThresh();        break;
            case AppState::CAL_MENU:             self->_drawCalMenu();           break;
            case AppState::SYSTEM_INFO:          self->_drawSystemInfo();        break;
            case AppState::CAL_MANUAL:           self->_drawCalmanual();         break;
            case AppState::EDIT_CAL_MANUAL:      self->_drawEditCalManual();     break;
            case AppState::CAL_DI:               self->_drawCal1();             break;
            case AppState::CAL_SALT:             self->_drawCal2();           break;
            case AppState::CAL_FINISH:           self->_drawCalFinish();         break;
            case AppState::CAL_CANCEL_CONFIRM:   self->_drawCalCancelConfirm();  break;
            // case AppState::SIM_STATUS:          self->_drawSimStatus();        break;
            case AppState::SIM_SENDING:         self->_drawSimSending();       break;
            case AppState::SIM_RESULT:          self->_drawSimResult();        break;
            case AppState::NETWORK_STATUS:      self->_drawNetworkStatus();  break;
            case AppState::SYSTEM_SETUP:        self->_drawSystemSetup();  break;
        }

        vTaskDelay(pdMS_TO_TICKS(DISPLAY_TASK_DELAY_MS));
    }
}

// ==========================================
// ดึงข้อมูลจาก Queue (ไม่บล็อค)
// ==========================================
void ScreenManager::_updateData() {
    xQueuePeek(sensorQueue, &_sensor, 0);
    xQueuePeek(gpsQueue,    &_gps,    0);
}

// ==========================================
// Helper: พิมพ์ตัวเลขแบบ fixed width
// ==========================================
void ScreenManager::_printPadded(float val, int decimals, int width) {
    char buf[16];
    dtostrf(val, width, decimals, buf);
    _lcd.print(buf);
}

// ==========================================
// Screens
// ==========================================
void ScreenManager::_drawStartup() {
    _lcd.setCursor(3, 1);
    _lcd.print("SALINITY METER");
    _lcd.setCursor(4, 2);
    _lcd.print("System Ready");
}

void ScreenManager::_drawMainScreen() {
    _lcd.setCursor(0, 0);
    _lcd.print("SAL : ");
    if (_sensor.adsValid) {
        _printPadded(_sensor.valPPT, 2, 5); 
        _lcd.print(" ppt     ");
    } else {
        _lcd.print("ERR       ");
    }

    _lcd.setCursor(0, 1);
    _lcd.print("EC  : ");
    if (_sensor.adsValid) {
        _printPadded(_sensor.valEC, 2, 5); 
        _lcd.print(" mS/cm   ");
    } else {
        _lcd.print("ERR       ");
    }

    _lcd.setCursor(0, 2);
    _lcd.print("Temp: ");
    if (_sensor.tempValid) {
        _printPadded(_sensor.tempC, 1, 5);
        _lcd.print(" ");
        _lcd.print((char)223); // สัญลักษณ์องศา (°)
        _lcd.print("C      ");
    } else {
        _lcd.print("ERR       ");
    }
    String netPrefix;
    String netStatus;
    
    if (NVSManager::config.networkMode == NET_MODE_SIM) {
        netPrefix = "SIM";
        if (SimTask::isConnected()) {
            netStatus = "OK " + String(SimTask::getSignalQuality());
        } else {
            netStatus = "Wait";
        }
    } else {
        netPrefix = "WIF"; 
        if (WifiTask::isConnected()) {
            netStatus = "OK " + String(WifiTask::getSignalQuality());
        } else {
            netStatus = "Wait";
        }
    }

    String gpsStatus;
    if (_gps.serialError) {
        gpsStatus = "SER_ERR";
    } else if (_gps.valid) {
        gpsStatus = "Fix " + String(_gps.satellites);
    } else {
        gpsStatus = "Wait";
    }

    char lcdBuf[21];
    snprintf(lcdBuf, sizeof(lcdBuf), "%-3.3s:%-6.6s|GPS:%-5.5s", 
            netPrefix.c_str(), netStatus.c_str(), gpsStatus.c_str());

    _lcd.setCursor(0, 3);
    _lcd.print(lcdBuf);
}

void ScreenManager::_drawMainMenu() {
    const int totalItems = 8; // จำนวนเมนูทั้งหมด
    const char* items[totalItems] = {
        "Read Temp",
        "Network Status", 
        "Setting", 
        "Read GPS",
        "LED Threshold", 
        "Calibrate Mode", 
        "System Info", 
        "Back"
    };

    // คำนวณ index เริ่มต้น
    int startIdx = _sm.menuIndex <= 1 ? 0 : min(_sm.menuIndex - 1, totalItems - 4); 
    
    // เตรียมตัวเลข "2/9" รอไว้ก่อนเลย
    char pageStr[8];
    snprintf(pageStr, sizeof(pageStr), "%d/%d", _sm.menuIndex + 1, totalItems);
    int pageLen = strlen(pageStr); // นับว่าตัวเลขกินที่กี่ตัวอักษร (เช่น "2/9" = 3 ตัว)

    // ==========================================
    // วาดข้อความเมนู
    // ==========================================
    for (int i = 0; i < 4; i++) {
        int idx = startIdx + i;
        _lcd.setCursor(0, i);
        
        if (idx < totalItems) {
            // เตรียมลูกศร
            const char* prefix = (_sm.menuIndex == idx) ? "> " : "  ";
            char buf[21]; 
            
            if (i == 0) {
                // บรรทัดบนสุด (i=0): ประกอบร่าง [ลูกศร] + [ชื่อเมนู] + [ตัวเลขหน้า]
                int nameSpace = 18 - pageLen; // คำนวณช่องว่างที่เหลือให้ชื่อเมนู
                
                // ใช้เทคนิค %-*.*s เพื่อหดช่องว่างของชื่อเมนูหลบให้ตัวเลขหน้าแบบอัตโนมัติ
                snprintf(buf, sizeof(buf), "%s%-*.*s%s", prefix, nameSpace, nameSpace, items[idx], pageStr);
                _lcd.print(buf);
            } else {
                // บรรทัดอื่นๆ: ประกอบร่าง [ลูกศร] + [ชื่อเมนู และเติมช่องว่างจนสุดขอบจอ]
                snprintf(buf, sizeof(buf), "%s%-18.18s", prefix, items[idx]);
                _lcd.print(buf);
            }
        } else {
            // ล้างบรรทัดที่ว่างเปล่า
            _lcd.print("                    ");
        }
    }
}

void ScreenManager::_drawReadTemp() {
    _lcd.setCursor(0, 0);
    _lcd.print("--- Temperature --- ");
    _lcd.setCursor(0, 1);
    _lcd.print("Celsius : ");
    _printPadded(_sensor.tempC, 2, 6);
    _lcd.print(" C");
    _lcd.setCursor(0, 2);
    _lcd.print("Fahren  : ");
    _printPadded((_sensor.tempC * 9.0f / 5.0f) + 32.0f, 2, 6);
    _lcd.print(" F");
    _lcd.setCursor(0, 3);
    _lcd.print("Kelvin  : ");
    _printPadded(_sensor.tempC + 273.15f, 2, 6);
    _lcd.print(" K");
}

void ScreenManager::_drawSystemSetup() {
    _lcd.setCursor(0, 0);
    _lcd.print("----- SETTING ------"); 

    _lcd.setCursor(0, 1);
    _lcd.print(_sm.menuIndex == 0 ? "> " : "  ");
    _lcd.print("Buzzer   : [");
    _lcd.print(NVSManager::config.isMuted ? "OFF]  " : "ON ]  ");

    _lcd.setCursor(0, 2);
    _lcd.print(_sm.menuIndex == 1 ? "> " : "  ");
    _lcd.print("Net Mode : [");
    _lcd.print(NVSManager::config.networkMode == NET_MODE_SIM ? "SIM]  " : "WIFI] ");

    _lcd.setCursor(0, 3);
    _lcd.print(_sm.menuIndex == 2 ? "> " : "  ");
    _lcd.print("Back              ");
}

// ==========================================
// READ_GPS
// ==========================================
void ScreenManager::_drawReadGPS() {
    char buf[21];
    _lcd.setCursor(0, 0);
    _lcd.print("--- GPS Status -----");

    if (_gps.valid) {
        // บรรทัด 2: ละติจูด
        _lcd.setCursor(0, 1);
        snprintf(buf, sizeof(buf), "Lat: %-14.5f", _gps.lat);
        _lcd.print(buf);

        // บรรทัด 3: ลองจิจูด
        _lcd.setCursor(0, 2);
        snprintf(buf, sizeof(buf), "Lon: %-14.5f", _gps.lng);
        _lcd.print(buf);

        // บรรทัด 4: จำนวนดาวเทียม และสถานะ
        _lcd.setCursor(0, 3);
        snprintf(buf, sizeof(buf), "Satellites: %-2d  [OK]", _gps.satellites);
        _lcd.print(buf);

    } else {
        _lcd.setCursor(0, 1);
        _lcd.print("Searching signal... ");
        
        // บรรทัด 3: คำแนะนำ
        _lcd.setCursor(0, 2);
        _lcd.print("Move to open sky.   ");
        
        // บรรทัด 4: โชว์จำนวนดาวเทียมที่เจอ (แม้จะยังไม่ Lock พิกัด)
        _lcd.setCursor(0, 3);
        snprintf(buf, sizeof(buf), "Satellites: %-2d      ", _gps.satellites);
        _lcd.print(buf);
    }
}

void ScreenManager::_drawThreshMenu() {
    int idx = _sm.menuIndex;
    _lcd.setCursor(0, 0);
    _lcd.print(idx == 0 ? "> " : "  ");
    _lcd.print("Green : ");
    _printPadded(NVSManager::thresh.green, 1, 5);
    _lcd.print(" ppt");

    _lcd.setCursor(0, 1);
    _lcd.print(idx == 1 ? "> " : "  ");
    _lcd.print("Yellow: ");
    _printPadded(NVSManager::thresh.yellow, 1, 5);
    _lcd.print(" ppt");

    _lcd.setCursor(0, 2);
    _lcd.print(idx == 2 ? "> " : "  ");
    _lcd.print("Red   : ");
    _printPadded(NVSManager::thresh.red, 1, 5);
    _lcd.print(" ppt");

    _lcd.setCursor(0, 3);
    _lcd.print(idx == 3 ? "> Back              " : "  Back              ");
}

void ScreenManager::_drawEditThresh() {
    _lcd.setCursor(0, 0);
    _lcd.print("--- Set Limit ---   ");

    _lcd.setCursor(0, 1);
    float val = 0.0f;
    
    if (_sm.editingColor == 'G') {
        _lcd.print("Max Green (G) limit ");
        val = NVSManager::thresh.green;
    } 
    else if (_sm.editingColor == 'Y') {
        _lcd.print("Max Yellow(Y) limit ");
        val = NVSManager::thresh.yellow;
    } 
    else {
        _lcd.print("Max Red (R) limit   ");
        val = NVSManager::thresh.red;
    }

    _lcd.setCursor(0, 2);
    _lcd.print("      [");
    _printPadded(val, 1, 5); 
    _lcd.print("]       ");

    _lcd.setCursor(0, 3);
    _lcd.print("   Click to Save    ");
}

void ScreenManager::_drawCalMenu() {
    const char* items[] = {
        "Auto Calibrate  ",  // 0
        "Manual Calibrate",  // 1
        "Send Calib Time ",  // 2
        "Back            "   // 3
    };
 
    for (int i = 0; i < 4; i++) {
        _lcd.setCursor(0, i);
        _lcd.print(_sm.menuIndex == i ? "> " : "  ");
        _lcd.print(items[i]);
    }
}


// ==========================================
// SYSTEM_INFO5
// ==========================================
void ScreenManager::_drawSystemInfo() {
    char buf[21];

    // บรรทัด 1: หัวข้อ
    _lcd.setCursor(0, 0);
    _lcd.print("--- System Info ----");

    // บรรทัด 2: Device ID 
    _lcd.setCursor(0, 1);
    snprintf(buf, sizeof(buf), "Device ID : %-9d", DEVICE_ID);
    _lcd.print(buf);

    // บรรทัด 3: Firmware Version (ดึงจาก #define)
    _lcd.setCursor(0, 2);
    // พิมพ์คำว่า "Firmware: " (8 ตัวอักษร) + ตัวแปรข้อความเผื่อไว้ 12 ตัวอักษร รวมเป็น 20 พอดี
    snprintf(buf, sizeof(buf), "Firmware  : %-12.12s", FW_VERSION); 
    _lcd.print(buf);

    // บรรทัด 4: เครดิต
    _lcd.setCursor(0, 3);
    _lcd.print("By AIOT LAB (KMITL) "); 
}


void ScreenManager::_drawCalmanual() {
    int idx = _sm.menuIndex;
    _lcd.setCursor(0, 0);
    _lcd.print(idx == 0 ? "> " : "  ");
    _lcd.print("Alpha : ");
    _printPadded(NVSManager::calibEC.alpha, 3, 5);
    _lcd.print("     ");

    _lcd.setCursor(0, 1);
    _lcd.print(idx == 1 ? "> " : "  ");
    _lcd.print("Beta  : ");
    _printPadded(NVSManager::calibEC.beta, 3, 5);
    _lcd.print("     ");

    _lcd.setCursor(0, 2);
    _lcd.print(idx == 2 ? "> Save And Send     " : "  Save And Send     ");

    _lcd.setCursor(0, 3);
    _lcd.print(idx == 3 ? "> Cancel            " : "  Cancel            ");
}

void ScreenManager::_drawEditCalManual() {
    _lcd.setCursor(0, 0);
    _lcd.print("--- Adjust Value ---");

    _lcd.setCursor(0, 1);
    if (_sm.editingColor == 'A') {
        _lcd.print("  Alpha (A) Multi.  ");
    } else {
        _lcd.print("   Beta (B) Offset  ");
    }

    _lcd.setCursor(0, 2);
    float val = (_sm.editingColor == 'A') ? NVSManager::calibEC.alpha : NVSManager::calibEC.beta;
    _lcd.print("      [");
    _printPadded(val, 3, 6);
    _lcd.print("]     ");

    _lcd.setCursor(0, 3);
    _lcd.print("  Click to Confirm  ");
}

void ScreenManager::_drawCal1() {
    char buf[21];
    _lcd.setCursor(0, 0);
    _lcd.print("1. Insert Water     "); 
    _lcd.setCursor(0, 1);
    _lcd.print("Target: 1.413 us/cm ");
    _lcd.setCursor(0, 2);
    snprintf(buf, sizeof(buf), "V:%-7.4f T:%-5.1fC", _sensor.voltEC, _sensor.tempC);
    _lcd.print(buf);
    _lcd.setCursor(0, 3);
    _lcd.print("Click to Cancel     ");
}

void ScreenManager::_drawCal2() {
    char buf[21];
    _lcd.setCursor(0, 0);
    _lcd.print("2. Insert Water     "); 
    _lcd.setCursor(0, 1);
    _lcd.print("Target: 12.88 ms/cm ");
    _lcd.setCursor(0, 2);
    snprintf(buf, sizeof(buf), "V:%-7.4f T:%-5.1fC", _sensor.voltEC, _sensor.tempC);
    _lcd.print(buf);
    _lcd.setCursor(0, 3);
    _lcd.print("Click to Cancel     ");
}

void ScreenManager::_drawCalFinish() {
    _lcd.setCursor(0, 0); 
    _lcd.print("--- Calibration --- ");
    
    _lcd.setCursor(0, 1); 
    _lcd.print("  Calibrate Finish! ");
    
    _lcd.setCursor(0, 2); 
    _lcd.print("     Success!!!     ");
    
    _lcd.setCursor(0, 3); 
    _lcd.print("                    ");
}

void ScreenManager::_drawCalCancelConfirm() {
    _lcd.setCursor(0, 0);
    _lcd.print("--- Confirm? ---    ");
    _lcd.setCursor(0, 1);
    _lcd.print(" Cancel Calibration?");
    _lcd.setCursor(0, 2);
    _lcd.print("                    ");
    _lcd.setCursor(0, 3);
    if (_sm.cancelSelect == 0) {
        _lcd.print("> YES          NO  ");
    } else {
        _lcd.print("  YES        > NO  ");
    }
}



// void ScreenManager::_drawSimStatus() {
//     _lcd.setCursor(0, 0);
//     _lcd.print("--- Send Data ---   ");

//     // แถว 1: สถานะ SIM + Signal Quality
//     _lcd.setCursor(0, 1);
//     int sig = SimTask::getSignalQuality();
//     if (!SimTask::isConnected()) {
//         _lcd.print("SIM : No Network... ");
//     } else if (sig == 99 || sig == 0) {
//         _lcd.print("SIM : No Signal (99) ");
//     } else {
//         _lcd.print("SIM : OK (");
//         _lcd.print(sig);
//         _lcd.print("/31)");
//     }

//     // แถว 2: สถานะ GPS
//     _lcd.setCursor(0, 2);
//     if (_gps.valid) {
//         _lcd.print("GPS : Lock  ");
//         _lcd.print(_gps.satellites);
//         _lcd.print("sat        ");
//     } else {
//         _lcd.print("GPS : No Lock...    ");
//     }

//     // แถว 3: ค่า Sensor
//     _lcd.setCursor(0, 3);
//     _lcd.print("S:");
//     _lcd.print(_sensor.currentPPT, 1);
//     _lcd.print("ppt T:");
//     _lcd.print(_sensor.currentTemp, 1);
//     _lcd.print("C  ");
// }

void ScreenManager::_drawSimSending() {
    _lcd.setCursor(0, 0); _lcd.print("--- Sending... ---  ");
    _lcd.setCursor(0, 1); _lcd.print(" Uploading data...  ");
    _lcd.setCursor(0, 2); _lcd.print(" Please wait...     ");
    _lcd.setCursor(0, 3); _lcd.print("                    ");
}

void ScreenManager::_drawSimResult() {
    _lcd.setCursor(0, 0); _lcd.print("--- Result ---      ");
    if (_sm.simLastSuccess) {
        _lcd.setCursor(0, 1); _lcd.print(" Send Success!  OK  ");
        _lcd.setCursor(0, 2);
        char buf[21];
        sprintf(buf, " HTTP: %-12d", _sm.simLastHttpCode);
        _lcd.print(buf);
    } else {
        _lcd.setCursor(0, 1); _lcd.print(" Send Failed!  X    ");
        _lcd.setCursor(0, 2);
        char buf[21];
        sprintf(buf, " HTTP: %-12d", _sm.simLastHttpCode);
        _lcd.print(buf);
    }
    _lcd.setCursor(0, 3); _lcd.print(" Click to go back   ");
}

// ==========================================
// NETWORK_STATUS
// ==========================================
void ScreenManager::_drawNetworkStatus() {
    char buf[21]; 

    if (NVSManager::config.networkMode == NET_MODE_WIFI) {
        // --- 🟢 โหมด Wi-Fi ---
        _lcd.setCursor(0, 0);
        _lcd.print("--- WIFI STATUS ----");

        _lcd.setCursor(0, 1);
        snprintf(buf, sizeof(buf), "SSID:%-15.15s", WIFI_SSID);
        _lcd.print(buf);

        _lcd.setCursor(0, 2);
        snprintf(buf, sizeof(buf), "PASS:%-15.15s", WIFI_PASS);
        _lcd.print(buf);

        _lcd.setCursor(0, 3);
        if (WifiTask::isConnected()) {
            snprintf(buf, sizeof(buf), "Stat:OK    RSSI:%-3d", WifiTask::getSignalQuality());
        } else {
            snprintf(buf, sizeof(buf), "Stat:WAIT  RSSI:-- ");
        }
        _lcd.print(buf);

    } else {
        // --- 🔵 โหมด SIM ---
        _lcd.setCursor(0, 0);
        _lcd.print("--- SIM STATUS -----");

        _lcd.setCursor(0, 1);
        snprintf(buf, sizeof(buf), "APN :%-15.15s", SIM_APN);
        _lcd.print(buf);

        _lcd.setCursor(0, 2);
        _lcd.print("Type:GPRS / SIM800L ");

        _lcd.setCursor(0, 3);
        if (SimTask::isConnected()) {
            snprintf(buf, sizeof(buf), "Stat:OK    CSQ :%-2d ", SimTask::getSignalQuality());
        } else {
            snprintf(buf, sizeof(buf), "Stat:WAIT  CSQ :--  ");
        }
        _lcd.print(buf);
    }
}

#endif