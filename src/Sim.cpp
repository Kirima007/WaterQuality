#define TINY_GSM_MODEM_SIM800

#include "Sim.h"
#include "StateMachine.h"
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>

// ==========================================
// Static Members
// ==========================================
volatile bool SimTask::_connected    = false;
volatile bool SimTask::_sendRequested = false;

bool SimTask::isConnected()  { return _connected; }
void SimTask::requestSend()  { _sendRequested = true; }

// ==========================================
// FreeRTOS Task Entry Point
// ==========================================
void SimTask::taskEntry(void* param) {
    disableCore0WDT();
    StateMachine* sm = static_cast<StateMachine*>(param);

    // --- เปิด SIM800L ---
    pinMode(MODEM_POWER_ON, OUTPUT);
    digitalWrite(MODEM_POWER_ON, HIGH);
    vTaskDelay(pdMS_TO_TICKS(1000));

    HardwareSerial simSerial(1);
    simSerial.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
    vTaskDelay(pdMS_TO_TICKS(3000));

    TinyGsm modem(simSerial);
    modem.restart();
    vTaskDelay(pdMS_TO_TICKS(2000));

    // --- รอจับสัญญาณ ---
    modem.waitForNetwork(60000L);

    // --- เชื่อมต่อ GPRS ---
    modem.gprsConnect(SIM_APN, "", "");
    _connected = modem.isGprsConnected();

    // ==========================================
    // Main Loop
    // ==========================================
    SensorData sensor{};
    GPSData    gps{};

    for (;;) {
        // อัปเดตสถานะ GPRS ทุก loop
        _connected = modem.isGprsConnected();
        if (!_connected) {
            modem.gprsConnect(SIM_APN, "", "");
            _connected = modem.isGprsConnected();
        }

        // รอ requestSend จาก StateMachine
        if (_sendRequested) {
            _sendRequested = false;

            // ดึงข้อมูลล่าสุด
            xQueuePeek(sensorQueue, &sensor, 0);
            xQueuePeek(gpsQueue,    &gps,    0);

            String payload  = _buildJson(sensor, gps);
            String response = "";
            int    httpCode = 0;
            bool   success  = false;

            if (_connected) {
                // สร้าง client ใหม่ทุกครั้ง
                TinyGsmClient gsmClient(modem);
                HttpClient    http(gsmClient, HTTP_HOST, HTTP_PORT);
                http.setTimeout(5000);

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
                response = http.responseBody();
                success  = (httpCode >= 200 && httpCode < 300);

                http.stop();
                gsmClient.stop();
            } else {
                // ไม่ได้ต่อ internet → แจ้ง fail ทันที
                httpCode = 0;
                success  = false;
            }

            // แจ้ง StateMachine ว่าส่งเสร็จแล้ว
            sm->onSimSendComplete(success, httpCode);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
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