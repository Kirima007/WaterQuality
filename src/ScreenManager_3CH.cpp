#include "ScreenManager.h"
#include "SimTask.h"

#if SENSOR_COUNT == 3

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

    for (;;) {
        self->_updateData();

        if (self->_sm.hasChanged()) {
            self->_lcd.clear();
        }

        switch (self->_sm.getState()) {
            case AppState::STARTUP:              self->_drawStartup();           break;
            case AppState::MAIN_SCREEN:          self->_drawMainScreen();        break;
            case AppState::MAIN_MENU:            self->_drawMainMenu();          break;
            case AppState::MONITOR_MENU:         self->_drawSensorSelectMenu();  break;
            case AppState::READ_GPS:             self->_drawReadGPS();           break;
            case AppState::THRESH_PARAM_MENU:    self->_drawSensorSelectMenu();  break;
            case AppState::THRESH_MENU:          self->_drawThreshMenu();        break;
            case AppState::EDIT_THRESH:          self->_drawEditThresh();        break;
            case AppState::CAL_PARAM_MENU:       self->_drawSensorSelectMenu();  break;
            case AppState::CAL_MENU:             self->_drawCalMenu();           break;
            case AppState::TEMP_CAL:             self->_drawTempCal();           break;
            case AppState::SYSTEM_INFO:          self->_drawSystemInfo();        break;
            case AppState::CAL_MANUAL:           self->_drawCalmanual();         break;
            case AppState::EDIT_CAL_MANUAL:      self->_drawEditCalManual();     break;
            case AppState::CAL_DI:               self->_drawCal1();             break;
            case AppState::CAL_SALT:             self->_drawCal2();           break;
            case AppState::CAL_FINISH:           self->_drawCalFinish();         break;
            case AppState::CAL_CANCEL_CONFIRM:   self->_drawCalCancelConfirm();  break;
            case AppState::SIM_SENDING:          self->_drawSimSending();        break;
            case AppState::SIM_RESULT:           self->_drawSimResult();         break;
            case AppState::NETWORK_STATUS:       self->_drawNetworkStatus();     break;
            case AppState::SYSTEM_SETUP:         self->_drawSystemSetup();       break;
            default: break;
        }

        vTaskDelay(pdMS_TO_TICKS(DISPLAY_TASK_DELAY_MS));
    }
}

void ScreenManager::_updateData() {
    xQueuePeek(sensorQueue, &_sensor, 0);
    xQueuePeek(gpsQueue,    &_gps,    0);
}

void ScreenManager::_printPadded(float val, int decimals, int width) {
    char buf[16];
    dtostrf(val, width, decimals, buf);
    _lcd.print(buf);
}

// ==========================================
// Screens
// ==========================================
void ScreenManager::_drawStartup() {
    // ตกแต่งหน้าจอ Startup ให้สวยงามและอยู่กึ่งกลาง (จอ 20x4)
    _lcd.setCursor(1, 0);  // 17 ตัวอักษร (20-17)/2 = 1.5 -> เริ่มที่ช่อง 1
    _lcd.print("Water Quality BK.");
    
    _lcd.setCursor(4, 1);  // 12 ตัวอักษร -> ขยับมาที่ช่อง 4 จะอยู่ตรงกลางพอดี
    _lcd.print("System Ready");
    
    // โชว์เวอร์ชันของเฟิร์มแวร์บรรทัดล่างสุด (จัดกึ่งกลางอัตโนมัติ)
    String fwMsg = String("Version: ") + FW_VERSION;
    int startPos = (20 - fwMsg.length()) / 2;
    _lcd.setCursor(startPos < 0 ? 0 : startPos, 3);
    _lcd.print(fwMsg);
}

void ScreenManager::_drawMainScreen() {
    // Row 0 & 1: Values
    if (_sm.currentParam == 0) {
        _lcd.setCursor(0, 0); _lcd.print("SAL : "); 
        if (_sensor.adsValid) { _printPadded(_sensor.valPPT, 2, 5); _lcd.print(" ppt     "); }
        else { _lcd.print("ERR       "); }

        _lcd.setCursor(0, 1); _lcd.print("EC  : "); 
        if (_sensor.adsValid) { _printPadded(_sensor.valEC, 2, 5); _lcd.print(" mS/cm   "); }
        else { _lcd.print("ERR       "); }
    } else if (_sm.currentParam == 1) {
        _lcd.setCursor(0, 0); _lcd.print("pH  : "); 
        if (_sensor.adsValid) { _printPadded(_sensor.valPH, 2, 5); _lcd.print(" pH      "); }
        else { _lcd.print("ERR       "); }

        _lcd.setCursor(0, 1); _lcd.print("Volt: "); 
        if (_sensor.adsValid) { _printPadded(_sensor.voltPH, 3, 5); _lcd.print(" V       "); }
        else { _lcd.print("ERR       "); }
    } else {
        _lcd.setCursor(0, 0); _lcd.print("DO  : "); 
        if (_sensor.adsValid) { _printPadded(_sensor.valDO, 2, 5); _lcd.print(" mg/L    "); }
        else { _lcd.print("ERR       "); }

        _lcd.setCursor(0, 1); _lcd.print("Volt: "); 
        if (_sensor.adsValid) { _printPadded(_sensor.voltDO, 3, 5); _lcd.print(" V       "); }
        else { _lcd.print("ERR       "); }
    }

    // Row 2: Temp
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

    // Row 3: Network & GPS Status (Ported from 1CH)
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
        gpsStatus = "ERR";
    } else if (_gps.valid) {
        gpsStatus = "OK " + String(_gps.satellites);
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
    const int totalItems = 8; // ลบ Back ออก
    const char* items[totalItems] = { 
        "Monitor Data",
        "Network Status",
        "GPS Status",
        "Setting",
        "Sensor Calibrate",
        "Temp Calibrate",
        "Alarm Limits",
        "System Info" 
    };
    int startIdx = _sm.menuIndex <= 1 ? 0 : min(_sm.menuIndex - 1, totalItems - 4); 
    
    char pageStr[8];
    snprintf(pageStr, sizeof(pageStr), "%d/%d", _sm.menuIndex + 1, totalItems);
    int pageLen = strlen(pageStr);

    for (int i = 0; i < 4; i++) {
        int idx = startIdx + i;
        _lcd.setCursor(0, i);
        if (idx < totalItems) {
            const char* prefix = (_sm.menuIndex == idx) ? "> " : "  ";
            char buf[21]; 
            if (i == 0) {
                int nameSpace = 18 - pageLen;
                snprintf(buf, sizeof(buf), "%s%-*.*s%s", prefix, nameSpace, nameSpace, items[idx], pageStr);
                _lcd.print(buf);
            } else {
                snprintf(buf, sizeof(buf), "%s%-18.18s", prefix, items[idx]);
                _lcd.print(buf);
            }
        } else {
            _lcd.print("                    ");
        }
    }
}

void ScreenManager::_drawSensorSelectMenu() {
    const char* items[] = {"Salinity (EC)     ", "pH                ", "DO                ", "Back              "};
    
    // แสดงเมนูทั้ง 4 บรรทัด (เลื่อนหน้าถ้าเกิน)
    for (int i = 0; i < 4; i++) {
        int displayIdx = (_sm.menuIndex < 4) ? i : i + 1; // ในที่นี้มีแค่ 4 บรรทัด เลยไม่จำเป็นต้องเลื่อน แต่เผื่อไว้
        if (i < 4) {
            _lcd.setCursor(0, i);
            _lcd.print(_sm.menuIndex == i ? "> " : "  ");
            _lcd.print(items[i]);
        }
    }
}

void ScreenManager::_drawReadGPS() {
    char buf[21];
    _lcd.setCursor(0, 0); _lcd.print("--- GPS Status -----");
    if (_gps.valid) {
        _lcd.setCursor(0, 1); snprintf(buf, sizeof(buf), "Lat: %-14.5f", _gps.lat); _lcd.print(buf);
        _lcd.setCursor(0, 2); snprintf(buf, sizeof(buf), "Lon: %-14.5f", _gps.lng); _lcd.print(buf);
        _lcd.setCursor(0, 3); snprintf(buf, sizeof(buf), "Satellites: %-2d  [OK]", _gps.satellites); _lcd.print(buf);
    } else {
        _lcd.setCursor(0, 1); _lcd.print("Searching signal... ");
        _lcd.setCursor(0, 2); _lcd.print("Move to open sky.   ");
        _lcd.setCursor(0, 3); snprintf(buf, sizeof(buf), "Satellites: %-2d      ", _gps.satellites); _lcd.print(buf);
    }
}

void ScreenManager::_drawThreshMenu() {
    const char* pfx = (_sm.currentParam == 0) ? "EC" : (_sm.currentParam == 1 ? "pH" : "DO");
    ThreshData* t = (_sm.currentParam == 0) ? &NVSManager::threshEC : (_sm.currentParam == 1 ? &NVSManager::threshPH : &NVSManager::threshDO);
    const char* unit = (_sm.currentParam == 0) ? " ppt" : (_sm.currentParam == 1 ? " pH " : " mgL");
    
    // Row 0: Green
    _lcd.setCursor(0, 0); _lcd.print(_sm.menuIndex == 0 ? "> " : "  "); 
    _lcd.print("Green : "); _printPadded(t->green, 1, 5); _lcd.print(unit);

    // Row 1: Yellow
    _lcd.setCursor(0, 1); _lcd.print(_sm.menuIndex == 1 ? "> " : "  "); 
    _lcd.print("Yellow: "); _printPadded(t->yellow, 1, 5); _lcd.print(unit);

    // Row 2: Red
    _lcd.setCursor(0, 2); _lcd.print(_sm.menuIndex == 2 ? "> " : "  "); 
    _lcd.print("Red   : "); _printPadded(t->red, 1, 5); _lcd.print(unit);

    // Row 3: Back + Sensor Name
    _lcd.setCursor(0, 3); _lcd.print(_sm.menuIndex == 3 ? "> " : "  ");
    _lcd.print("Back            "); 
    _lcd.setCursor(16, 3);
    _lcd.print("["); _lcd.print(pfx); _lcd.print("]");
}

void ScreenManager::_drawEditThresh() {
    _lcd.setCursor(0, 0); _lcd.print("--- Set Limit ---   ");
    
    ThreshData* t = (_sm.currentParam == 0) ? &NVSManager::threshEC : (_sm.currentParam == 1 ? &NVSManager::threshPH : &NVSManager::threshDO);
    float val = 0.0f;
    
    _lcd.setCursor(0, 1);
    if (_sm.editingColor == 'G') { _lcd.print("Max Green (G) limit "); val = t->green; } 
    else if (_sm.editingColor == 'Y') { _lcd.print("Max Yellow(Y) limit "); val = t->yellow; } 
    else { _lcd.print("Max Red (R) limit   "); val = t->red; }

    _lcd.setCursor(0, 2); _lcd.print("      ["); _printPadded(val, 1, 5); _lcd.print("]       ");
    _lcd.setCursor(0, 3); _lcd.print("   Click to Save    ");
}

void ScreenManager::_drawCalMenu() {
    const char* pfx = (_sm.currentParam == 0) ? "EC" : (_sm.currentParam == 1 ? "pH" : "DO");
    const char* items[] = { 
        "Auto Calibrate  ", 
        "Manual Calibrate", 
        "Send Calib Time ", 
        "Back            " 
    };
    
    for (int i = 0; i < 4; i++) {
        _lcd.setCursor(0, i);
        _lcd.print(_sm.menuIndex == i ? "> " : "  ");
        _lcd.print(items[i]);
    }

    // ชื่อเซ็นเซอร์มุมขวาล่าง
    _lcd.setCursor(16, 3);
    _lcd.print("["); _lcd.print(pfx); _lcd.print("]");
}

void ScreenManager::_drawTempCal() {
    _lcd.setCursor(0, 0); _lcd.print("--- Temp Calib ---  ");
    
    _lcd.setCursor(0, 1); _lcd.print(" Offset : [");
    _printPadded(NVSManager::tempOffset, 1, 5); _lcd.print("]  ");
    
    char buf[21];
    snprintf(buf, sizeof(buf), " Current: %-5.1f", _sensor.tempC);
    _lcd.setCursor(0, 2); _lcd.print(buf);
    _lcd.print((char)223); // สัญลักษณ์องศา (°)
    _lcd.print("C  ");
    
    _lcd.setCursor(0, 3); _lcd.print("   Click to Save    ");
}

void ScreenManager::_drawCal1() {
    char buf[21];
    const char* targets[] = { "Target: 1.413 mS/cm", "Target: 4.0 pH     ", "Target: 0.0 mg/L   " };
    const char* name[]   = { "1.Calib EC          ", "1.Calib pH          ", "1.Calib DO          " };
    
    _lcd.setCursor(0, 0); _lcd.print(name[_sm.currentParam]); 
    _lcd.setCursor(0, 1); _lcd.print(targets[_sm.currentParam]);
    
    float v = (_sm.currentParam == 0) ? _sensor.voltEC : (_sm.currentParam == 1 ? _sensor.voltPH : _sensor.voltDO);
    snprintf(buf, sizeof(buf), "V:%-7.4f T:%-5.1fC", v, _sensor.tempC);
    _lcd.setCursor(0, 2); _lcd.print(buf);
    
    _lcd.setCursor(0, 3); _lcd.print("Click to Cancel     ");
}

void ScreenManager::_drawCal2() {
    char buf[21];
    const char* targets[] = { "Target: 12.88 mS/cm", "Target: 7.0 pH     ", "Target: 8.0 mg/L   " };
    const char* name[]   = { "2.Calib EC          ", "2.Calib pH          ", "2.Calib DO          " };
    
    _lcd.setCursor(0, 0); _lcd.print(name[_sm.currentParam]); 
    _lcd.setCursor(0, 1); _lcd.print(targets[_sm.currentParam]);
    
    float v = (_sm.currentParam == 0) ? _sensor.voltEC : (_sm.currentParam == 1 ? _sensor.voltPH : _sensor.voltDO);
    snprintf(buf, sizeof(buf), "V:%-7.4f T:%-5.1fC", v, _sensor.tempC);
    _lcd.setCursor(0, 2); _lcd.print(buf);
    
    _lcd.setCursor(0, 3); _lcd.print("Click to Cancel     ");
}

void ScreenManager::_drawCalFinish() {
    _lcd.setCursor(0, 0); 
    _lcd.print("--- Calibration --- ");
    
    if (_sm.calibSuccess) {
        _lcd.setCursor(0, 1); 
        _lcd.print("  Calibrate Finish! ");
        _lcd.setCursor(0, 2); 
        _lcd.print("     Success!!!     ");
    } else {
        _lcd.setCursor(0, 1); 
        _lcd.print("  Calibrate Failed! ");
        _lcd.setCursor(0, 2); 
        _lcd.print("    Math Error!!    ");
    }
    
    _lcd.setCursor(0, 3); 
    _lcd.print("   Please Wait...   ");
}

void ScreenManager::_drawCalCancelConfirm() {
    _lcd.setCursor(0, 0); _lcd.print("--- Confirm? ---    ");
    _lcd.setCursor(0, 1); _lcd.print(" Cancel Calibration?");
    _lcd.setCursor(0, 2); _lcd.print("                    ");
    _lcd.setCursor(0, 3);
    if (_sm.cancelSelect == 0) _lcd.print("  > YES        NO   ");
    else                       _lcd.print("    YES      > NO   ");
}

void ScreenManager::_drawCalmanual() {
    const char* pfx = (_sm.currentParam == 0) ? "EC" : (_sm.currentParam == 1 ? "pH" : "DO");
    CalibData* c = (_sm.currentParam == 0) ? &NVSManager::calibEC : (_sm.currentParam == 1 ? &NVSManager::calibPH : &NVSManager::calibDO);
    
    _lcd.setCursor(0, 0); _lcd.print(_sm.menuIndex == 0 ? "> " : "  "); _lcd.print("Alpha : "); _printPadded(c->alpha, 3, 5); _lcd.print("     ");
    _lcd.setCursor(0, 1); _lcd.print(_sm.menuIndex == 1 ? "> " : "  "); _lcd.print("Beta  : "); _printPadded(c->beta, 3, 5); _lcd.print("     ");
    _lcd.setCursor(0, 2); _lcd.print(_sm.menuIndex == 2 ? "> Save And Send     " : "  Save And Send     ");
    _lcd.setCursor(0, 3); _lcd.print(_sm.menuIndex == 3 ? "> Cancel            " : "  Cancel            ");

    // ชื่อเซ็นเซอร์มุมขวาล่าง (ใส่ก้ามปู)
    _lcd.setCursor(16, 3);
    _lcd.print("["); _lcd.print(pfx); _lcd.print("]");
}

void ScreenManager::_drawEditCalManual() {
    _lcd.setCursor(0, 0); _lcd.print("--- Adjust Value ---");
    _lcd.setCursor(0, 1);
    if (_sm.editingColor == 'A') _lcd.print("  Alpha (A) Multi.  ");
    else _lcd.print("   Beta (B) Offset  ");
    
    _lcd.setCursor(0, 2);
    CalibData* c = (_sm.currentParam == 0) ? &NVSManager::calibEC : (_sm.currentParam == 1 ? &NVSManager::calibPH : &NVSManager::calibDO);
    float val = (_sm.editingColor == 'A') ? c->alpha : c->beta;
    _lcd.print("      ["); _printPadded(val, 3, 6); _lcd.print("]     ");
    
    _lcd.setCursor(0, 3); _lcd.print("  Click to Confirm  ");
}

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

void ScreenManager::_drawSystemSetup() {
    _lcd.setCursor(0, 0);
    _lcd.print("----- SETTING ------"); 

    _lcd.setCursor(0, 1);
    _lcd.print(_sm.menuIndex == 0 ? "> " : "  ");
    _lcd.print("Net Mode : [");
    _lcd.print(NVSManager::config.networkMode == NET_MODE_SIM ? "SIM]  " : "WIFI] ");

    _lcd.setCursor(0, 2);
    _lcd.print(_sm.menuIndex == 1 ? "> " : "  ");
    _lcd.print("Buzzer   : [");
    _lcd.print(NVSManager::config.isMuted ? "OFF]  " : "ON ]  ");

    _lcd.setCursor(0, 3);
    _lcd.print(_sm.menuIndex == 2 ? "> " : "  ");
    _lcd.print("Back              ");
}

void ScreenManager::_drawNetworkStatus() {
    char buf[21]; 
    if (NVSManager::config.networkMode == NET_MODE_WIFI) {
        _lcd.setCursor(0, 0); _lcd.print("--- WIFI STATUS ----");
        _lcd.setCursor(0, 1); snprintf(buf, sizeof(buf), "SSID:%-15.15s", WIFI_SSID); _lcd.print(buf);
        _lcd.setCursor(0, 2); snprintf(buf, sizeof(buf), "PASS:%-15.15s", WIFI_PASS); _lcd.print(buf);
        _lcd.setCursor(0, 3);
        if (WifiTask::isConnected()) snprintf(buf, sizeof(buf), "Stat:OK    RSSI:%-3d", WifiTask::getSignalQuality());
        else snprintf(buf, sizeof(buf), "Stat:WAIT  RSSI:-- ");
        _lcd.print(buf);
    } else {
        _lcd.setCursor(0, 0); _lcd.print("--- SIM STATUS -----");
        _lcd.setCursor(0, 1); snprintf(buf, sizeof(buf), "APN :%-15.15s", SIM_APN); _lcd.print(buf);
        _lcd.setCursor(0, 2); _lcd.print("Type:GPRS / SIM800L ");
        _lcd.setCursor(0, 3);
        if (SimTask::isConnected()) snprintf(buf, sizeof(buf), "Stat:OK    CSQ :%-2d ", SimTask::getSignalQuality());
        else snprintf(buf, sizeof(buf), "Stat:WAIT  CSQ :--  ");
        _lcd.print(buf);
    }
}

void ScreenManager::_drawSystemInfo() {
    char buf[21];
    _lcd.setCursor(0, 0); _lcd.print("--- System Info ----");
    _lcd.setCursor(0, 1); snprintf(buf, sizeof(buf), "Dev ID  : %-9d", DEVICE_ID); _lcd.print(buf);
    _lcd.setCursor(0, 2); snprintf(buf, sizeof(buf), "Firmware: %-10.10s", FW_VERSION); _lcd.print(buf);
    _lcd.setCursor(0, 3); _lcd.print("By AIOT LAB (KMITL) "); 
}

#endif