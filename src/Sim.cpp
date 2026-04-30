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
volatile bool SimTask::_sendCalibRequested = false;  // ← เพิ่ม
volatile int  SimTask::_signalQuality      = 0;

bool SimTask::isConnected()      { return _connected; }
int  SimTask::getSignalQuality() { return _signalQuality; }
void SimTask::requestSend()      { _sendRequested      = true; }
void SimTask::requestSendCalib() { _sendCalibRequested = true; }  // ← เพิ่ม

TinyGsm       modem(simSerial);
TinyGsmClient gsmClient(modem);
static SemaphoreHandle_t modemMutex = nullptr;

// ==========================================
// Helper: ส่ง HTTP POST (ใช้ร่วมกันทั้งสอง endpoint)
// ==========================================
static bool _doPost(const String& path, const String& payload, StateMachine* sm, bool isCalib) {
    int  httpCode = 0;
    bool success  = false;

    if (xSemaphoreTake(modemMutex, portMAX_DELAY) == pdTRUE) {
        HttpClient http(gsmClient, HTTP_HOST, HTTP_PORT);
        http.setTimeout(2000);

        http.beginRequest();
        http.post(path);
        http.sendHeader("Content-Type",  "application/json");
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

        _connected     = (networkOk && gprsOk);
        _signalQuality = _connected ? modem.getSignalQuality() : 0;

        // รอบ 2: ส่ง Sensor Data
        if (_sendRequested) {
            _sendRequested = false;

            if (!_connected) {
                sm->onSimSendComplete(false, 0);
            } else {
                xQueuePeek(sensorQueue, &sensor, 0);
                xQueuePeek(gpsQueue,    &gps,    0);
                String payload = _buildJson(sensor, gps);
                _doPost(HTTP_PATH, payload, sm, false);
            }
        }

        // รอบ 3: ส่ง Calibration Data ← ใหม่
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

    JsonObject address = doc["address"].to<JsonObject>();
    if (gps.valid) {
        address["x"] = String(gps.lat, 6);
        address["y"] = String(gps.lng, 6);
    } else {
        address["x"] = nullptr;
        address["y"] = nullptr;
    }

    String payload;
    serializeJson(doc, payload);
    return payload;
}

// ==========================================
// JSON: Calibration Data ← ใหม่
// {
//   "id": 1,
//   "alpha": 1.023,
//   "beta": -0.015,
//   "v_di": 0.0004,
//   "v_salt": 0.4746,
//   "t_salt": 23.5
// }
// ==========================================
String SimTask::_buildCalibJson() {
    JsonDocument doc;
    doc["id"]     = DEVICE_ID;
    doc["alpha"]  = serialized(String(NVSManager::calib.alpha,  4));
    doc["beta"]   = serialized(String(NVSManager::calib.beta,   4));
    doc["v_di"]   = serialized(String(NVSManager::calib.v_di,   4));
    doc["v_salt"] = serialized(String(NVSManager::calib.v_salt, 4));
    doc["t_salt"] = serialized(String(NVSManager::calib.t_salt, 2));

    String payload;
    serializeJson(doc, payload);
    return payload;
}