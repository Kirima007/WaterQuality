#define TINY_GSM_MODEM_SIM800
#define simSerial Serial1

#include "Sim.h"
#include "StateMachine.h"
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>

// ==========================================
// Static Members
// ==========================================
volatile bool SimTask::_connected          = false;
volatile bool SimTask::_sendRequested      = false;
volatile bool SimTask::_sendCalibRequested = false;
volatile int  SimTask::_signalQuality      = 0;

bool SimTask::isConnected()      { return _connected; }
int  SimTask::getSignalQuality() { return _signalQuality; }
void SimTask::requestSend()      { _sendRequested      = true; }
void SimTask::requestSendCalib() { _sendCalibRequested = true; }

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
// FreeRTOS Task Entry Point
// ==========================================
void SimTask::taskEntry(void* param) {
    disableCore0WDT();
    StateMachine* sm = static_cast<StateMachine*>(param);
    modemMutex = xSemaphoreCreateMutex();

    // --- เปิด SIM800L ---
    pinMode(MODEM_PWRKEY,   OUTPUT);
    pinMode(MODEM_RST,      OUTPUT);
    pinMode(MODEM_POWER_ON, OUTPUT);
    digitalWrite(MODEM_PWRKEY,   LOW);
    digitalWrite(MODEM_RST,      HIGH);
    digitalWrite(MODEM_POWER_ON, HIGH);
    vTaskDelay(pdMS_TO_TICKS(1000));

    simSerial.begin(9600, SERIAL_8N1, MODEM_RX, MODEM_TX);
    vTaskDelay(pdMS_TO_TICKS(3000));

    modem.restart();
    vTaskDelay(pdMS_TO_TICKS(2000));

    // ==========================================
    // Main Loop
    // ==========================================
    SensorData sensor{};
    GPSData    gps{};
    int failCount = 0; // ตัวนับสำหรับทำ Hard Reset

    for (;;) {
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
                        // แจ้ง StateMachine ว่าส่ง Fail (ใช้ -2 เพื่อบอกว่าเป็นเพราะ GPS ไม่ติด)
                        sm->onSimSendComplete(false, -2); 
                    } else {
                        // ถ้า GPS ติด ค่อยแพ็ก JSON แล้วส่ง
                        String payload = _buildJson(sensor, gps);
                        _doPost(HTTP_PATH, payload, sm, false);
                    }
                } else {
                    sm->onSimSendComplete(false, -1); // คิวว่าง Fail
                }
            }
        }

        // รอบ 3: ส่ง Calibration Data (ปกติ Calib ไม่จำเป็นต้องบังคับมี GPS ก็ได้)
        if (_sendCalibRequested) {
            _sendCalibRequested = false;

            if (!_connected) {
                sm->onSimSendComplete(false, 0);
            } else {
                String payload = _buildCalibJson();
                _doPost(HTTP_PATH_CALIB, payload, sm, true);
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
    doc["id"]       = DEVICE_ID;
    doc["salinity"] = serialized(String(sensor.currentPPT,  2));
    doc["temp"]     = serialized(String(sensor.currentTemp, 1));

    // เนื่องจากเรากรอง gps.valid ไปแล้วก่อนส่ง ตรงนี้จะมีค่าเสมอ
    JsonObject address = doc["address"].to<JsonObject>();
    address["x"] = String(gps.lat, 6);
    address["y"] = String(gps.lng, 6);

    String payload;
    serializeJson(doc, payload);
    return payload;
}

// ==========================================
// JSON: Calibration Data
// ==========================================
String SimTask::_buildCalibJson() {
    JsonDocument doc;
    doc["id"]     = DEVICE_ID;

    String payload;
    serializeJson(doc, payload);
    return payload;
}