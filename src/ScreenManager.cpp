#include "ScreenManager.h"

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
            case AppState::CAL_DI:               self->_drawCalDI();             break;
            case AppState::CAL_SALT:             self->_drawCalSalt();           break;
            case AppState::CAL_FINISH:           self->_drawCalFinish();         break;
            case AppState::CAL_CANCEL_CONFIRM:   self->_drawCalCancelConfirm();  break;
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

    _lcd.setCursor(0, 1);
    _lcd.print("Sal : ");
    _printPadded(_sensor.currentPPT, 2, 7);
    _lcd.print(" ppt");

    _lcd.setCursor(0, 2);
    _lcd.print("Temp: ");
    _printPadded(_sensor.currentTemp, 1, 5);
    _lcd.print(" C  ");

    _lcd.setCursor(0, 3);
    if (_gps.valid) {
        _lcd.print("GPS:");
        _lcd.print(_gps.lat, 3);
        _lcd.print(",");
        _lcd.print(_gps.lng, 3);
    } else {
        _lcd.print("GPS : Wait Lock...  ");
    }
}

void ScreenManager::_drawMainMenu() {
    const char* items[] = {
        "Read Temp         ",
        "Read GPS          ",
        "LED Threshold     ",
        "Calibrate Mode    ",
        "Back              "
    };

    int startIdx = max(0, min(_sm.menuIndex - 3, 1));
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
    _lcd.print("Green: ");
    _printPadded(NVSManager::thresh.green, 1, 5);
    _lcd.print(" ppt");

    _lcd.setCursor(0, 1);
    _lcd.print(idx == 1 ? "> " : "  ");
    _lcd.print("Blue : ");
    _printPadded(NVSManager::thresh.red, 1, 5);
    _lcd.print(" ppt");

    _lcd.setCursor(0, 2);
    _lcd.print(idx == 2 ? "> " : "  ");
    _lcd.print("Red  : ");
    _printPadded(NVSManager::thresh.red, 1, 5);
    _lcd.print(" ppt");

    _lcd.setCursor(0, 3);
    _lcd.print(idx == 3 ? "> Back              " : "  Back              ");
}

void ScreenManager::_drawEditThresh() {
    _lcd.setCursor(0, 0);
    _lcd.print("--- Set Limit ---   ");
    _lcd.setCursor(0, 1);
    if (_sm.editingColor == 'G') {
        _lcd.print("Max Green (G) limit ");
    } else {
        _lcd.print("Max Blue  (B) limit ");
    }
    _lcd.setCursor(0, 2);
    float val = (_sm.editingColor == 'G')
        ? NVSManager::thresh.green
        : NVSManager::thresh.red;
    _lcd.print("      [");
    _printPadded(val, 1, 5);
    _lcd.print("]      ");
    _lcd.setCursor(0, 3);
    _lcd.print("   Click to Save    ");
}

void ScreenManager::_drawCalDI() {
    _lcd.setCursor(0, 0);
    _lcd.print("1. Insert DI Water  ");
    _lcd.setCursor(0, 1);
    _lcd.print("V:");
    _lcd.print(_sensor.currentVolt, 4);
    _lcd.print(" T:");
    _lcd.print(_sensor.currentTemp, 1);
    _lcd.print("C ");
    _lcd.setCursor(0, 2);
    _lcd.print("Click  to Cancel    ");
    _lcd.setCursor(0, 3);
    _lcd.print(">> Hold to Record   ");
}

void ScreenManager::_drawCalSalt() {
    _lcd.setCursor(0, 0);
    _lcd.print("2. Insert 35ppt Sol.");
    _lcd.setCursor(0, 1);
    _lcd.print("V:");
    _lcd.print(_sensor.currentVolt, 4);
    _lcd.print(" T:");
    _lcd.print(_sensor.currentTemp, 1);
    _lcd.print("C ");
    _lcd.setCursor(0, 2);
    _lcd.print("Click  to Cancel    ");
    _lcd.setCursor(0, 3);
    _lcd.print(">> Hold to Record   ");
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