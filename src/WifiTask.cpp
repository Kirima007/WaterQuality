#include "WifiTask.h"
#include "StateMachine.h"
#include "NVSManager.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>

// ==========================================
// Static Members
// ==========================================
volatile bool WifiTask::_connected          = false;
volatile bool WifiTask::_sendRequested      = false;
volatile bool WifiTask::_sendCalibRequested = false;
volatile uint8_t WifiTask::_reqSensorIdx    = 0;
volatile uint8_t WifiTask::_reqCalibIdx     = 0;
volatile int  WifiTask::_signalQuality      = 0;

bool WifiTask::isConnected()      { return _connected; }
int  WifiTask::getSignalQuality() { return _signalQuality; }
void WifiTask::requestSend(uint8_t sensorIndex) { 
    _reqSensorIdx = sensorIndex;
    _sendRequested = true; 
}
void WifiTask::requestSendCalib(uint8_t sensorIndex) { 
    _reqCalibIdx = sensorIndex;
    _sendCalibRequested = true; 
}

// ==========================================
// Helper: ส่ง HTTP POST ผ่าน Wi-Fi
// ==========================================
bool WifiTask::_doPost(const String& path, const String& payload, StateMachine* sm) {
    bool success = false;
    int httpCode = 0;

    if (WiFi.status() == WL_CONNECTED) {
        WiFiClient wifiClient; // สร้างตัวแทนของ Wi-Fi
        HttpClient http(wifiClient, HTTP_HOST, HTTP_PORT); // ให้ HttpClient คุยผ่าน Wi-Fi
        
        http.setTimeout(5000); // รอตอบกลับ 5 วินาที

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
        http.responseBody(); // flush ขยะทิ้ง
        success  = (httpCode >= 200 && httpCode < 300);

        http.stop();
        wifiClient.stop();
    }

    // แจ้งผลกลับไปที่ StateMachine
    sm->onSimSendComplete(success, httpCode);
    return success;
}

// ==========================================
// FreeRTOS Task Entry Point
// ==========================================
void WifiTask::taskEntry(void* param) {
    StateMachine* sm = static_cast<StateMachine*>(param);

    SensorData sensor{};
    GPSData    gps{};
    unsigned long lastWifiCheck = 0;

    for (;;) {
        // --- 1. เช็คว่าผู้ใช้เปิดโหมด Wi-Fi อยู่ไหม ---
        // ถ้าใช้โหมด SIM อยู่ ให้ Task นี้ตัดการเชื่อมต่อและหลับไปเลย
        if (NVSManager::config.networkMode == NET_MODE_SIM) {
            if (WiFi.status() == WL_CONNECTED) {
                WiFi.disconnect(true);
                WiFi.mode(WIFI_OFF);
                _connected = false;
            }
            vTaskDelay(pdMS_TO_TICKS(1000)); // หลับยาวๆ ไม่กวนระบบ
            continue;
        }

        // --- 2. ระบบเชื่อมต่อ Wi-Fi ---
        if (WiFi.status() != WL_CONNECTED) {
            _connected = false;
            // พยายามต่อ Wi-Fi ทุกๆ 5 วินาที
            if (millis() - lastWifiCheck > 5000) {
                Serial.println("[WIFI] Connecting to Hotspot...");
                WiFi.mode(WIFI_STA);
                WiFi.setTxPower(WIFI_POWER_19_5dBm); // ตั้งกำลังส่ง Wi-Fi สูงสุด (19.5 dBm)
                WiFi.begin(WIFI_SSID, WIFI_PASS); // ดึงชื่อ/รหัส จาก Config.h
                lastWifiCheck = millis();
            }
        } else {
            _connected = true;
            _signalQuality = WiFi.RSSI(); // ความแรงสัญญาณ Wi-Fi
        }

        // --- 3. ตรวจสอบคำสั่งให้ส่งข้อมูล Sensor ---
        if (_sendRequested) {
            _sendRequested = false;

            if (!_connected) {
                sm->onSimSendComplete(false, 0);
            } else {
                if (xQueuePeek(sensorQueue, &sensor, 0) == pdTRUE) {
                    xQueuePeek(gpsQueue, &gps, 0); 
                    
                    //ถ้า GPS ไม่ติด ยกเลิกการส่ง
                    if (!gps.valid) {
                        Serial.println("[WIFI] Abort sending: GPS no fix!");
                        sm->onSimSendComplete(false, -2); 
                    } else {
                        String payload = _buildJson(sensor, gps);
                        _doPost(HTTP_PATH, payload, sm);
                    }
                } 
                else {
                    sm->onSimSendComplete(false, -1);
                }
            }
        }

        // --- 4. ตรวจสอบคำสั่งให้ส่งข้อมูล Calibrate ---
        if (_sendCalibRequested) {
            _sendCalibRequested = false;

            if (!_connected) {
                sm->onSimSendComplete(false, 0);
            } else {
                String payload = _buildCalibJson();
                String path = HTTP_PATH_CALIB_SALINITY;
                if (_reqCalibIdx == 1) path = HTTP_PATH_CALIB_PH;
                else if (_reqCalibIdx == 2) path = HTTP_PATH_CALIB_O2;

                _doPost(path, payload, sm);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // หน่วงเวลา Loop สั้นๆ
    }
}

// ==========================================
// JSON: Sensor Data
// ==========================================
String WifiTask::_buildJson(const SensorData& sensor, const GPSData& gps) {
    JsonDocument doc;
    doc["id"]       = DEVICE_ID;
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

    String payload;
    serializeJson(doc, payload);
    return payload;
}

// ==========================================
// JSON: Calibration Data
// ==========================================
String WifiTask::_buildCalibJson() {
    JsonDocument doc;
    doc["id"]     = DEVICE_ID;
    String payload;
    serializeJson(doc, payload);
    return payload;
}