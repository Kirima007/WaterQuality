// ต้อง define modem type ก่อน include TinyGSM เสมอ
#define TINY_GSM_MODEM_SIM800

#include "Sim.h"
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>

// ==========================================
// FreeRTOS Task Entry Point
// ==========================================
void SimTask::taskEntry(void* param) {

    // --- เปิดไฟ SIM800L ---
    pinMode(MODEM_POWER_ON, OUTPUT);
    digitalWrite(MODEM_POWER_ON, HIGH);
    vTaskDelay(pdMS_TO_TICKS(1000));

    // --- UART1 สำหรับ SIM800L ---
    HardwareSerial simSerial(1);
    simSerial.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
    vTaskDelay(pdMS_TO_TICKS(3000));  // รอ modem boot

    TinyGsm modem(simSerial);
    modem.restart();
    vTaskDelay(pdMS_TO_TICKS(2000));

    // --- เชื่อมต่อ GPRS (วนจนสำเร็จ) ---
    while (!modem.gprsConnect(SIM_APN, "", "")) {
        vTaskDelay(pdMS_TO_TICKS(5000));
    }

    // ==========================================
    // Main Loop
    // ==========================================
    SensorData sensor{};
    GPSData    gps{};
    String     response;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(SIM_PUBLISH_MS));

        // --- เช็ค GPRS ยังต่ออยู่ไหม ---
        if (!modem.isGprsConnected()) {
            modem.gprsConnect(SIM_APN, "", "");
            vTaskDelay(pdMS_TO_TICKS(3000));
            continue;
        }

        // --- ดึงข้อมูลล่าสุดจาก Queue ---
        xQueuePeek(sensorQueue, &sensor, 0);
        xQueuePeek(gpsQueue,    &gps,    0);

        // --- สร้าง JSON และส่ง HTTP ---
        String payload = _buildJson(sensor, gps);

        // สร้าง client ใหม่ทุกครั้ง (หลีกเลี่ยง connection hang)
        TinyGsmClient gsmClient(modem);
        HttpClient    http(gsmClient, HTTP_HOST, HTTP_PORT);
        http.setTimeout(5000);

        http.beginRequest();
        http.post(HTTP_PATH);
        http.sendHeader("Content-Type", "application/json");
        http.sendHeader("Accept",       "application/json");
        http.sendHeader("Connection",   "close");
        http.sendHeader("Content-Length", payload.length());
        http.beginBody();
        http.print(payload);
        http.endRequest();

        int httpCode = http.responseStatusCode();
        response     = http.responseBody();

        http.stop();
        gsmClient.stop();
    }
}

// ==========================================
// สร้าง JSON Payload
// รูปแบบ:
// {
//   "id": 1,
//   "salinity": 15.23,
//   "temp": 28.5,
//   "address": { "x": "13.736717", "y": "100.523186" }
// }
// ==========================================
String SimTask::_buildJson(const SensorData& sensor, const GPSData& gps) {
    JsonDocument doc;

    doc["id"]       = DEVICE_ID;
    doc["salinity"] = serialized(String(sensor.currentPPT,  2));
    doc["temp"]     = serialized(String(sensor.currentTemp, 1));

    // GPS address
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