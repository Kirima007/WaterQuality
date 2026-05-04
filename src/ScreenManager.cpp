#include "ScreenManager.h"
#include "Sim.h"

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
            case AppState::CAL_DI:               self->_drawCalDI();             break;
            case AppState::CAL_SALT:             self->_drawCalSalt();           break;
            case AppState::CAL_FINISH:           self->_drawCalFinish();         break;
            case AppState::CAL_CANCEL_CONFIRM:   self->_drawCalCancelConfirm();  break;
            // case AppState::SIM_STATUS:          self->_drawSimStatus();        break;
            case AppState::SIM_SENDING:         self->_drawSimSending();       break;
            case AppState::SIM_RESULT:          self->_drawSimResult();        break;
            case AppState::NETWORK_MENU:        self->_drawNetworkMenu();  break;

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
    _lcd.print("---SALINITY METER---");

    // ==========================================
    // บรรทัดที่ 2: ค่า EC (ความเค็ม)
    // ==========================================
    _lcd.setCursor(0, 1);
    _lcd.print("EC  : ");
    _printPadded(_sensor.currentPPT, 2, 5); 
    _lcd.print(" ppt    "); // เติมช่องว่างเผื่อล้างตัวเลขเก่าที่อาจตกค้าง

    // ==========================================
    // บรรทัดที่ 3: ค่า Temp (อุณหภูมิ)
    // ==========================================
    _lcd.setCursor(0, 2);
    _lcd.print("Temp: ");
    _printPadded(_sensor.currentTemp, 1, 5);
    _lcd.print(" ");
    _lcd.print((char)223); // พิมพ์สัญลักษณ์องศา (°) ของจอ LCD
    _lcd.print("C   ");   // เติมช่องว่างเผื่อล้างตัวเลขเก่า

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
        netPrefix = "WIF"; // ใช้ WIF แทน WIFI เพื่อประหยัดโควต้าตัวอักษร
        if (WifiTask::isConnected()) {
            netStatus = "OK " + String(WifiTask::getSignalQuality()); // แสดงค่า RSSI ติดลบ (เช่น -65)
        } else {
            netStatus = "Wait";
        }
    }

    String gpsStatus;
    if (_gps.valid) {
        gpsStatus = "Fix " + String(_gps.satellites);
    } else {
        gpsStatus = "Wait";
    }
    char lcdBuf[21];
    snprintf(lcdBuf, sizeof(lcdBuf), "%-3s:%-6s|GPS:%-5s", 
             netPrefix.c_str(), 
             netStatus.c_str(), 
             gpsStatus.c_str());

    _lcd.setCursor(0, 3);
    _lcd.print(lcdBuf);
}

void ScreenManager::_drawMainMenu() {
    const char* items[] = {
        "Read Temp         ",
        "Network Mode      ",
        "Read GPS          ",
        "LED Threshold     ",
        "Calibrate Mode    ",
        "System Info       ",
        "Back              "
    };

    int startIdx = _sm.menuIndex <= 1 ? 0 : min(_sm.menuIndex - 1, 3);
    for (int i = 0; i < 4; i++) {
        _lcd.setCursor(0, i);
        int idx = startIdx + i;
        _lcd.print(_sm.menuIndex == idx ? "> " : "  ");
        _lcd.print(items[idx]);
    }
}

void ScreenManager::_drawReadTemp() {
    _lcd.setCursor(0, 0);
    _lcd.print("--- Temperature --- ");
    _lcd.setCursor(0, 1);
    _lcd.print("Celsius : ");
    _printPadded(_sensor.currentTemp, 2, 6);
    _lcd.print(" C");
    _lcd.setCursor(0, 2);
    _lcd.print("Fahren  : ");
    _printPadded((_sensor.currentTemp * 9.0f / 5.0f) + 32.0f, 2, 6);
    _lcd.print(" F");
    _lcd.setCursor(0, 3);
    _lcd.print("Kelvin  : ");
    _printPadded(_sensor.currentTemp + 273.15f, 2, 6);
    _lcd.print(" K");
}

void ScreenManager::_drawNetworkMenu() {
    _lcd.setCursor(0, 0);
    _lcd.print("--- Network Mode ---"); 

    _lcd.setCursor(0, 1);
    // เว้นช่องว่างให้พอดี 20 ตัวอักษรเพื่อล้างจอไปในตัว
    if (NVSManager::config.networkMode == NET_MODE_SIM) {
        _lcd.print("  Select : [ SIM ]  "); 
    } else {
        _lcd.print("  Select : [ WIFI]  ");
    }

    _lcd.setCursor(0, 2);
    _lcd.print("                    "); // บรรทัดว่าง

    _lcd.setCursor(0, 3);
    _lcd.print("   Click to Save    ");
}

void ScreenManager::_drawReadGPS() {
    _lcd.setCursor(0, 0);
    _lcd.print("--- GPS Data ---    ");
    if (_gps.valid) {
        _lcd.setCursor(0, 1);
        _lcd.print("Lat: ");
        _lcd.print(_gps.lat, 5);
        _lcd.setCursor(0, 2);
        _lcd.print("Lon: ");
        _lcd.print(_gps.lng, 5);
        _lcd.setCursor(0, 3);
        _lcd.print("Satellites: ");
        _lcd.print(_gps.satellites);
        _lcd.print("   ");
    } else {
        _lcd.setCursor(0, 1);
        _lcd.print("Searching Pos...    ");
        _lcd.setCursor(0, 2);
        _lcd.print("Move to open sky.   ");
        _lcd.setCursor(0, 3);
        _lcd.print("                    ");
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


void ScreenManager::_drawSystemInfo() {
    _lcd.setCursor(0, 0);
    _lcd.print("--- System Info ----");

    _lcd.setCursor(0, 1);
    char buf1[21];
    sprintf(buf1, "Device ID: %-9d", DEVICE_ID);
    _lcd.print(buf1);

    _lcd.setCursor(0, 2);
    char buf2[21];
    sprintf(buf2, "Sensor CH: %-9s", "CH0");
    _lcd.print(buf2);

    _lcd.setCursor(0, 3);
    _lcd.print("by MARID (AIOT LAB) ");
}


void ScreenManager::_drawCalmanual() {
    int idx = _sm.menuIndex;
    _lcd.setCursor(0, 0);
    _lcd.print(idx == 0 ? "> " : "  ");
    _lcd.print("Alpha : ");
    _printPadded(NVSManager::calib.alpha, 3, 5);

    _lcd.setCursor(0, 1);
    _lcd.print(idx == 1 ? "> " : "  ");
    _lcd.print("Beta  : ");
    _printPadded(NVSManager::calib.beta, 3, 5);

    _lcd.setCursor(0, 2);
    _lcd.print(idx == 2 ? "> Save And SendTime" : "  Save And SendTime");
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
    float val = (_sm.editingColor == 'A') ? NVSManager::calib.alpha : NVSManager::calib.beta;
    _lcd.print("      [");
    _printPadded(val, 3, 6);
    _lcd.print("]     ");

    _lcd.setCursor(0, 3);
    _lcd.print("  Click to Confirm  ");
}

void ScreenManager::_drawCalDI() {
    _lcd.setCursor(0, 0);
    _lcd.print("1. Insert Water     ");
    _lcd.setCursor(0, 1);
    _lcd.print("Target: 1.413 us/cm");
    _lcd.setCursor(0, 2);
    _lcd.print("V:");
    _lcd.print(_sensor.currentVolt, 4);
    _lcd.print(" T:");
    _lcd.print(_sensor.currentTemp, 1);
    _lcd.print("C   ");
    _lcd.setCursor(0, 3);
    _lcd.print("Click  to Cancel    ");
}

void ScreenManager::_drawCalSalt() {
    _lcd.setCursor(0, 0);
    _lcd.print("2. Insert Water     ");
    _lcd.setCursor(0, 1);
    _lcd.print("Target: 12.88 ms/cm");
    _lcd.setCursor(0, 2);
    _lcd.print("V:");
    _lcd.print(_sensor.currentVolt, 4);
    _lcd.print(" T:");
    _lcd.print(_sensor.currentTemp, 1);
    _lcd.print("C   ");
    _lcd.setCursor(0, 3);
    _lcd.print("Click  to Cancel    ");
}

void ScreenManager::_drawCalFinish() {
    _lcd.setCursor(0, 0); _lcd.print("--- Calibration --- ");
    _lcd.setCursor(0, 1); _lcd.print("   Calibrating...   ");
    _lcd.setCursor(0, 2); _lcd.print("   Saving to NVS    ");
    _lcd.setCursor(0, 3); _lcd.print("                    ");
    
    vTaskDelay(pdMS_TO_TICKS(1500)); // รอ 1.5 วิ

    _lcd.setCursor(0, 1); _lcd.print("  Calibrate Finish! ");
    _lcd.setCursor(0, 2); _lcd.print("     Success!!!     ");
    requestSound(SoundEvent::SUCCESS);
    
    vTaskDelay(pdMS_TO_TICKS(2000)); // รอ 2 วิ
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