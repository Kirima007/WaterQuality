#include "WifiTask.h"
#include "StateMachine.h"
#include "NVSManager.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <Update.h>

// ==========================================
// Static Members
// ==========================================
volatile bool WifiTask::_connected          = false;
volatile bool WifiTask::_sendRequested      = false;
volatile bool WifiTask::_sendCalibRequested = false;
volatile bool WifiTask::_checkOtaRequested  = false;
volatile bool WifiTask::_startOtaRequested  = false;
String        WifiTask::_otaUrl             = "";
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

void WifiTask::requestCheckOta() {
    _checkOtaRequested = true;
}

void WifiTask::requestStartOta(const String& url) {
    _otaUrl = url;
    _startOtaRequested = true;
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
    unsigned long lastWifiAttempt = 0;
    bool wifiStarted = false;                    // สั่ง WiFi.begin() ไปแล้วหรือยัง
    const unsigned long WIFI_RETRY_MS = 30000;   // ต่อไม่ติดนานเกินนี้ ค่อยเริ่มเชื่อมต่อใหม่

    for (;;) {
        // --- 1. เช็คว่าผู้ใช้เปิดโหมด Wi-Fi อยู่ไหม ---
        // ถ้าใช้โหมด SIM อยู่ ให้ปิด Wi-Fi (รวมกรณีที่ยังต่อไม่ติดแต่กำลังพยายามอยู่) แล้วหลับไป
        if (NVSManager::config.networkMode == NET_MODE_SIM) {
            if (wifiStarted) {
                WiFi.disconnect(true);
                WiFi.mode(WIFI_OFF);
                wifiStarted = false;
            }
            _connected = false;
            vTaskDelay(pdMS_TO_TICKS(1000)); // หลับยาวๆ ไม่กวนระบบ
            continue;
        }

        // --- 2. ระบบเชื่อมต่อ Wi-Fi ---
        if (WiFi.status() != WL_CONNECTED) {
            _connected = false;

            if (!wifiStarted) {
                // เริ่มเชื่อมต่อครั้งแรก — ให้ระบบ reconnect เองอัตโนมัติเมื่อหลุด
                Serial.println("[WIFI] Connecting to Hotspot...");
                WiFi.mode(WIFI_STA);
                WiFi.setAutoReconnect(true);
                WiFi.begin(WIFI_SSID, WIFI_PASS); // ดึงชื่อ/รหัส จาก Config.h
                // ลดกำลังส่งเพื่อลดกระแสพีค (เดิม 19.5 dBm) — ตั้งหลัง begin() เพื่อให้ค่าไม่ถูกรีเซ็ตทับ
                // ถ้า Hotspot อยู่ไกลแล้วต่อไม่ติด ค่อยขยับขึ้นเป็น WIFI_POWER_15dBm
                WiFi.setTxPower(WIFI_POWER_13dBm);
                wifiStarted = true;
                lastWifiAttempt = millis();
            }
            else if (millis() - lastWifiAttempt > WIFI_RETRY_MS) {
                // ต่อไม่ติดนานเกิน 30 วิ → เริ่มใหม่แบบสะอาด
                // ต้อง disconnect ก่อน ไม่งั้นได้ error "sta is connecting, cannot set config"
                Serial.println("[WIFI] Retry connecting...");
                WiFi.disconnect();
                vTaskDelay(pdMS_TO_TICKS(100));
                WiFi.begin(WIFI_SSID, WIFI_PASS);
                WiFi.setTxPower(WIFI_POWER_13dBm);
                lastWifiAttempt = millis();
            }
            // นอกนั้น: กำลังเชื่อมต่ออยู่ ปล่อยให้มันทำงานไป ไม่สั่ง begin() ซ้ำ
        } else {
            _connected = true;
            _signalQuality = WiFi.RSSI(); // ความแรงสัญญาณ Wi-Fi
            lastWifiAttempt = millis();   // ถ้าหลุด ให้ auto-reconnect ทำงานก่อน 30 วิ ค่อยบังคับเริ่มใหม่
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

        // --- 5. ตรวจสอบคำสั่งเช็คเวอร์ชัน OTA ---
        if (_checkOtaRequested) {
            _checkOtaRequested = false;

            if (!_connected) {
                sm->onOtaCheckComplete(false);
            } else {
                WiFiClient wifiClient; 
                HttpClient http(wifiClient, HTTP_HOST, HTTP_PORT); 
                http.setTimeout(5000); 

                http.beginRequest();
                http.get(HTTP_PATH_OTA_VERSION);
                http.sendHeader("Accept", "application/json");
                http.sendHeader("Connection", "close");
                http.endRequest();

                int httpCode = http.responseStatusCode();
                bool success = false;

                if (httpCode >= 200 && httpCode < 300) {
                    String payload = http.responseBody();
                    Serial.println("[OTA] Raw JSON from Server:");
                    Serial.println(payload);

                    JsonDocument doc;
                    DeserializationError error = deserializeJson(doc, payload);
                    
                    if (!error) {
                        String latestVer = doc["latest_version"].as<String>();
                        Serial.println("[OTA] Parsed Version: " + latestVer);
                        
                        sm->otaLatestVersion = latestVer;
                        sm->otaDownloadUrl   = doc["firmware_url"].as<String>();
                        sm->otaMD5           = doc["md5"] | ""; // ดึงค่า MD5 (ถ้ามี)
                        // ถ้าเวอร์ชันจากเว็บ ไม่ตรงกับที่มีอยู่ในเครื่อง = มีอัปเดตใหม่
                        sm->otaHasUpdate = (latestVer != String(FW_VERSION));
                        success = true;
                    } else {
                        Serial.println("[OTA] JSON Parse Failed: " + String(error.c_str()));
                    }
                } else {
                    http.responseBody(); // เคลียร์ข้อความตอบกลับทิ้ง
                }

                http.stop();
                wifiClient.stop();
                
                sm->onOtaCheckComplete(success);
            }
        }

        // --- 6. ตรวจสอบคำสั่งเริ่มดาวน์โหลด OTA ---
        if (_startOtaRequested) {
            _startOtaRequested = false;
            if (_connected) {
                // แยกเฉพาะ Path ออกจาก URL เต็ม 
                // (สมมติ http://161.246.157.210/api/fw/firmware_v1_5.bin -> เอาแค่ /api/fw/...)
                int pathIndex = _otaUrl.indexOf("/", 7);
                String path = _otaUrl.substring(pathIndex);

                WiFiClient wifiClient;
                HttpClient http(wifiClient, HTTP_HOST, HTTP_PORT);

                http.beginRequest();
                http.get(path);
                http.endRequest();

                int httpCode = http.responseStatusCode();
                bool otaSuccess = false;

                if (httpCode == 200) {
                    int contentLength = http.contentLength();
                    if (contentLength > 0 && Update.begin(contentLength)) {
                        
                        if (sm->otaMD5.length() > 0) {
                            Update.setMD5(sm->otaMD5.c_str()); // สั่งให้ ESP32 ตรวจสอบความถูกต้องระดับ MD5
                        }

                        size_t written = 0;
                        uint8_t buff[1024];
                        
                        while ((http.connected() || http.available()) && written < contentLength) {
                            size_t size = http.available();
                            if (size) {
                                int c = http.read(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
                                Update.write(buff, c);
                                written += c;
                                
                                sm->otaProgress = (written * 100) / contentLength;
                            }
                            vTaskDelay(pdMS_TO_TICKS(5));
                        }

                        if (written == contentLength && Update.end() && Update.isFinished()) {
                            Serial.println("[OTA] Success! Rebooting...");
                            vTaskDelay(pdMS_TO_TICKS(1000));
                            otaSuccess = true;
                            ESP.restart(); 
                        } else {
                            Update.abort();
                        }
                    }
                }
                http.stop();
                wifiClient.stop();

                // ถ้าล้มเหลว (เช่น เน็ตหลุด หรือ Update ไม่จบ) แจ้ง StateMachine ให้เด้งกลับหน้าเมนู
                if (!otaSuccess) {
                    Serial.println("[OTA] Download Failed or Interrupted!");
                    sm->onOtaDownloadFailed();
                }
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

    String payload;
    serializeJson(doc, payload);
    return payload;
}

// ==========================================
// JSON: Calibration Data
// ==========================================
String WifiTask::_buildCalibJson() {
    JsonDocument doc;
    doc["id"]     = NVSManager::config.deviceId;
    String payload;
    serializeJson(doc, payload);
    return payload;
}