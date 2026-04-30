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
volatile bool SimTask::_connected     = false;
volatile bool SimTask::_sendRequested = false;
volatile int  SimTask::_signalQuality = 0;

bool SimTask::isConnected()      { return _connected; }
int  SimTask::getSignalQuality() { return _signalQuality; }
void SimTask::requestSend()      { _sendRequested = true; }

// modem อยู่ global ไม่กิน Stack ของ Task
TinyGsm       modem(simSerial);
TinyGsmClient gsmClient(modem);

// Mutex ป้องกัน modem ถูกใช้พร้อมกัน (copy จาก reference)
static SemaphoreHandle_t modemMutex = nullptr;

// ==========================================
// FreeRTOS Task Entry Point
// ==========================================
void SimTask::taskEntry(void* param) {
    disableCore0WDT();

    StateMachine* sm = static_cast<StateMachine*>(param);

    // สร้าง Mutex
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

        // --- รอบ 1: เช็คและต่อ Network + GPRS ผ่าน Mutex ---
        if (xSemaphoreTake(modemMutex, portMAX_DELAY) == pdTRUE) {

            networkOk = modem.isNetworkConnected();
            if (!networkOk) {
                networkOk = modem.waitForNetwork(60000L);
            }

            if (networkOk) {
                gprsOk = modem.isGprsConnected();
                if (!gprsOk) {
                    gprsOk = modem.gprsConnect(SIM_APN, "", "");
                }
            }

            xSemaphoreGive(modemMutex);
        }

        _connected     = (networkOk && gprsOk);
        _signalQuality = _connected ? modem.getSignalQuality() : 0;

        // --- รอบ 2: เช็ค sendRequested ---
        if (_sendRequested) {
            _sendRequested = false;

            // ถ้าไม่มีเน็ต → แจ้งทันที ไม่ต้องรอ HTTP timeout
            if (!_connected) {
                sm->onSimSendComplete(false, 0);
                vTaskDelay(pdMS_TO_TICKS(200));
                continue;
            }

            // ดึงข้อมูลล่าสุด
            xQueuePeek(sensorQueue, &sensor, 0);
            xQueuePeek(gpsQueue,    &gps,    0);

            String payload  = _buildJson(sensor, gps);
            int    httpCode = 0;
            bool   success  = false;

            // ส่ง HTTP ผ่าน Mutex (รอบ 2 แยกจากรอบ Network)
            if (xSemaphoreTake(modemMutex, portMAX_DELAY) == pdTRUE) {

                HttpClient http(gsmClient, HTTP_HOST, HTTP_PORT);
                http.setTimeout(2000);  // ← 2 วิ เหมือน reference

                http.beginRequest();
                http.post(HTTP_PATH);
                http.sendHeader("Content-Type",   "application/json");
                http.sendHeader("Accept",          "application/json");
                http.sendHeader("Connection",      "close");
                http.sendHeader("Content-Length",  payload.length());
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

            sm->onSimSendComplete(success, httpCode);
        }

        vTaskDelay(pdMS_TO_TICKS(200)); // เหมือน reference
    }
}

// ==========================================
// สร้าง JSON Payload
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