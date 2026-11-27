#define TINY_GSM_MODEM_SIM800
#define simSerial Serial1

#include "SimTask.h"
#include "StateMachine.h"
#include "ResetLog.h"
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>

// ==========================================
// Static Members
// ==========================================
volatile bool SimTask::_connected          = false;
volatile bool SimTask::_sendRequested      = false;
volatile bool SimTask::_sendCalibRequested = false;
volatile uint8_t SimTask::_reqSensorIdx    = 0;
volatile uint8_t SimTask::_reqCalibIdx     = 0;
volatile int  SimTask::_signalQuality      = 0;

bool SimTask::isConnected()      { return _connected; }
int  SimTask::getSignalQuality() { return _signalQuality; }
void SimTask::requestSend(uint8_t sensorIndex) { 
    _reqSensorIdx = sensorIndex;
    _sendRequested = true; 
}
void SimTask::requestSendCalib(uint8_t sensorIndex) { 
    _reqCalibIdx = sensorIndex;
    _sendCalibRequested = true; 
}

TinyGsm       modem(simSerial);
TinyGsmClient gsmClient(modem);
static SemaphoreHandle_t modemMutex = nullptr;

// ==========================================
// Helper: ส่ง HTTP POST
// ==========================================
static bool _doPost(const String& path, const String& payload, StateMachine* sm, bool isCalib) {
    int  httpCode = 0;
    bool success  = false;

    if (xSemaphoreTake(modemMutex, portMAX_DELAY) == pdTRUE) {
        HttpClient http(gsmClient, HTTP_HOST, HTTP_PORT);
        http.setTimeout(2000);

        http.beginRequest();
        http.post(path);
        http.sendHeader("Content-Type",   "application/json");
        http.sendHeader("Accept",         "application/json");
        http.sendHeader("Connection",     "close");
        http.sendHeader("Content-Length", payload.length());
        http.beginBody();
        http.print(payload);
        http.endRequest();

        httpCode = http.responseStatusCode();
        http.responseBody(); // flush
        success  = (httpCode >= 200 && httpCode < 300);

        http.stop();
        gsmClient.stop();

        xSemaphoreGive(modemMutex);
    }

    // แจ้ง StateMachine เสมอ ไม่ว่าจะ sensor หรือ calib
    sm->onSimSendComplete(success, httpCode);
    return success;
}

// ==========================================
// Helper: เปิด / ปิดไฟ SIM800L
// GPIO23 (MODEM_POWER_ON) คือขา EN ของ buck ~4V ที่จ่ายไฟ VBAT ให้ SIM800L (ดู schematic T-Call V1.4)
// ==========================================
static void _modemPowerOn() {
    Serial.println("[SIM800L] Power ON");
    pinMode(MODEM_PWRKEY,   OUTPUT);
    pinMode(MODEM_RST,      OUTPUT);
    pinMode(MODEM_POWER_ON, OUTPUT);
    digitalWrite(MODEM_PWRKEY,   LOW);
    digitalWrite(MODEM_RST,      HIGH);
    digitalWrite(MODEM_POWER_ON, HIGH);
    vTaskDelay(pdMS_TO_TICKS(1000));

    simSerial.begin(9600, SERIAL_8N1, MODEM_RX, MODEM_TX);
    vTaskDelay(pdMS_TO_TICKS(3000));

    if (xSemaphoreTake(modemMutex, portMAX_DELAY) == pdTRUE) {
        modem.restart();
        xSemaphoreGive(modemMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
}

static void _modemPowerOff() {
    Serial.println("[SIM800L] Power OFF (WiFi mode)");
    if (xSemaphoreTake(modemMutex, portMAX_DELAY) == pdTRUE) {
        modem.poweroff();   // AT+CPOWD=1: ปิดแบบปกติ ออกจากเครือข่ายเรียบร้อยก่อนตัดไฟ
        xSemaphoreGive(modemMutex);
    }
    simSerial.end();
    // ไม่ปล่อยขาสัญญาณค้าง HIGH เข้า SIM800L ที่ไม่มีไฟ (กันไฟย้อนเข้าทางขา I/O)
    pinMode(MODEM_TX,  INPUT);
    pinMode(MODEM_RST, INPUT);
    digitalWrite(MODEM_POWER_ON, LOW);
}

// ==========================================
// FreeRTOS Task Entry Point
// ==========================================
void SimTask::taskEntry(void* param) {
    // หมายเหตุ: เดิมเรียก disableCore0WDT() ตรงนี้ แต่ใน Arduino core 3.x มันถอด IDLE0 ออกจาก WDT
    // โดยไม่ถอด idle hook ทำให้ log "esp_task_wdt_reset: task not found" รัว → ย้ายไปตั้งค่าใน setup() แทน
    StateMachine* sm = static_cast<StateMachine*>(param);
    modemMutex = xSemaphoreCreateMutex();

#ifdef TEST_NO_MODEM
    // [ทดสอบชั่วคราว] ไม่จ่ายไฟให้ SIM800L เพื่อดูว่าอาการ reset หายไหม
    pinMode(MODEM_POWER_ON, OUTPUT);
    digitalWrite(MODEM_POWER_ON, LOW);
    for (;;) vTaskDelay(portMAX_DELAY);
#endif

    // --- เริ่มต้นปิดไฟ SIM800L ไว้ก่อน แล้วค่อยเปิดเมื่ออยู่โหมด SIM ---
    // (เดิมเปิดตลอดแม้อยู่โหมด Wi-Fi → SIM800L ลงทะเบียนเครือข่ายและกระชากไฟเป็นระยะโดยไม่จำเป็น)
    pinMode(MODEM_POWER_ON, OUTPUT);
    digitalWrite(MODEM_POWER_ON, LOW);
    bool modemOn = false;

    // ==========================================
    // Main Loop
    // ==========================================
    SensorData sensor{};
    GPSData    gps{};
    int failCount = 0; // ตัวนับสำหรับทำ Hard Reset

    for (;;) {

        if (NVSManager::config.networkMode == NET_MODE_WIFI) {
            // ถ้าใช้ Wi-Fi อยู่ ให้ปิดไฟ SIM800L และล้างคำสั่งทิ้งไปเลย
            if (modemOn) {
                _modemPowerOff();
                modemOn = false;
            }
            _connected = false;
            _signalQuality = 0;
            _sendRequested = false;
            _sendCalibRequested = false;
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue; // ข้ามการทำงานรอบนี้ไปเลย
        }

        // โหมด SIM: เปิดไฟ SIM800L ถ้ายังไม่เปิด (ตอนบูต หรือเพิ่งสลับจาก Wi-Fi)
        if (!modemOn) {
            _modemPowerOn();
            modemOn = true;
            failCount = 0;
        }

        bool networkOk = false;
        bool gprsOk    = false;

        // รอบ 1: เช็ค / ต่อ Network + GPRS
        if (xSemaphoreTake(modemMutex, portMAX_DELAY) == pdTRUE) {
            networkOk = modem.isNetworkConnected();
            if (!networkOk) networkOk = modem.waitForNetwork(60000L);

            if (networkOk) {
                gprsOk = modem.isGprsConnected();
                if (!gprsOk) gprsOk = modem.gprsConnect(SIM_APN, "", "");
            }
            xSemaphoreGive(modemMutex);
        }

        _connected = (networkOk && gprsOk);

        // --- ระบบกู้ชีพ (Auto-Recovery) ---
        if (!_connected) {
            failCount++;
            if (failCount > 10) { 
                Serial.println("[SIM800L] Hang detected! Hard Resetting...");
                digitalWrite(MODEM_RST, LOW);
                vTaskDelay(pdMS_TO_TICKS(100));
                digitalWrite(MODEM_RST, HIGH);
                vTaskDelay(pdMS_TO_TICKS(3000));
                
                if (xSemaphoreTake(modemMutex, portMAX_DELAY) == pdTRUE) {
                    modem.restart();
                    xSemaphoreGive(modemMutex);
                }
                failCount = 0;
            }
        } else {
            failCount = 0;
            _signalQuality = modem.getSignalQuality();
        }

        // รอบ 2: ส่ง Sensor Data
        if (_sendRequested) {
            _sendRequested = false;

            if (!_connected) {
                sm->onSimSendComplete(false, 0);
            } else {
                if (xQueuePeek(sensorQueue, &sensor, 0) == pdTRUE) {
                    xQueuePeek(gpsQueue, &gps, 0); 
                    
                    // 🚨 กฎเหล็ก: ถ้า GPS ไม่ติด ยกเลิกการส่งทันที!
                    if (!gps.valid) {
                        Serial.println("[SIM800L] Abort sending: GPS no fix!");
                        sm->onSimSendComplete(false, -2); 
                    } else {
                        String payload = _buildJson(sensor, gps);
                        _doPost(HTTP_PATH, payload, sm, false);
                    }
                } else {
                    sm->onSimSendComplete(false, -1); // คิวว่าง Fail
                }
            }
        }

        // รอบ 3: ส่ง Calibration Data
        if (_sendCalibRequested) {
            _sendCalibRequested = false;

            if (!_connected) {
                sm->onSimSendComplete(false, 0);
            } else {
                String payload = _buildCalibJson();
                String path = HTTP_PATH_CALIB_SALINITY;
                if (_reqCalibIdx == 1) path = HTTP_PATH_CALIB_PH;
                else if (_reqCalibIdx == 2) path = HTTP_PATH_CALIB_O2;
                
                _doPost(path, payload, sm, true);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// ==========================================
// JSON: Sensor Data
// ==========================================
String SimTask::_buildJson(const SensorData& sensor, const GPSData& gps) {
    JsonDocument doc;
    doc["id"]       = NVSManager::config.deviceId;
    doc["temp"]     = serialized(String(sensor.tempC, 1));

    if (_reqSensorIdx == 0) {
        doc["salinity"] = serialized(String(sensor.valPPT, 2));
    } 
#if SENSOR_COUNT == 3
    else if (_reqSensorIdx == 1) {
        doc["ph"] = serialized(String(sensor.valPH, 2));
    } else if (_reqSensorIdx == 2) {
        doc["o2"] = serialized(String(sensor.valDO, 2));
    }
#endif

    JsonObject address = doc["address"].to<JsonObject>();
    address["x"] = String(gps.lat, 6);
    address["y"] = String(gps.lng, 6);

    // สาเหตุรีเซตรอบนี้ + จำนวนรีเซตผิดปกติสะสม (ดูเครื่องในสนามได้จาก server)
    doc["rst"]     = ResetLog::reasonName();
    doc["rst_err"] = ResetLog::errorCount();

    String payload;
    serializeJson(doc, payload);
    return payload;
}

// ==========================================
// JSON: Calibration Data
// ==========================================
String SimTask::_buildCalibJson() {
    JsonDocument doc;
    doc["id"]     = NVSManager::config.deviceId;
    String payload;
    serializeJson(doc, payload);
    return payload;
}