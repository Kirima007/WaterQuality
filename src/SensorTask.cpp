#include "SensorTask.h"
#include "SensorMath.h"
#include "NVSManager.h"
#include "config.h"
#include <Adafruit_ADS1X15.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ==========================================
// Helper Function: ฟังก์ชันกรองสัญญาณรบกวน (Moving Average)
// ==========================================
static int16_t readADSAvg(Adafruit_ADS1115& ads, uint8_t channel, int samples = 10) {
    long sum = 0;
    for (int i = 0; i < samples; i++) {
        sum += ads.readADC_SingleEnded(channel);
        // หน่วงเวลาสั้นๆ 5ms ระหว่างอ่านแต่ละครั้ง เพื่อหลบสัญญาณรบกวนความถี่สูง
        vTaskDelay(pdMS_TO_TICKS(5)); 
    }
    return (int16_t)(sum / samples);
}

// ==========================================
// FreeRTOS Task
// ==========================================
void SensorTask::taskEntry(void* param) {
    // --- Init Hardware ใน Task ---
    
    // ADS1115
    Adafruit_ADS1115 ads;
    bool adsPresent = ads.begin();
    if (!adsPresent) {
        Serial.println("[SENSOR] WARNING: ADS1115 not found!");
    }
    ads.setGain(GAIN_ONE);

    // DS18B20 (Temperature)
    OneWire oneWire(ONE_WIRE);
    DallasTemperature ds(&oneWire);
    ds.begin();
    ds.setWaitForConversion(false);  // non-blocking mode ช่วยให้ RTOS ไม่สะดุด

    // --- ตัวแปรภายใน Task ---
    SensorData data{};
    data.tempC = 25.0f; // ตั้งค่าเริ่มต้นเป็น 25 องศา เผื่อเซ็นเซอร์อ่านไม่ได้
    data.tempValid = false;
    data.adsValid = adsPresent;
    bool tempRequestPending = false;

    // Request อุณหภูมิครั้งแรก
    ds.requestTemperatures();
    tempRequestPending = true;

    // ==========================================
    // Main Loop
    // ==========================================
    for (;;) {
        // --- 1. จัดการอุณหภูมิ ---
        if (tempRequestPending) {
            float t = ds.getTempCByIndex(0);

            // เช็คว่าอ่านค่าได้จริง
            if (t != DEVICE_DISCONNECTED_C && t > -20.0f && t < 80.0f) {
                data.tempC = t + NVSManager::tempOffset;
                data.tempValid = true;
            } else {
                // Fallback: ถ้าระบบหาเซ็นเซอร์อุณหภูมิไม่เจอ ให้ล็อกค่าไว้ที่ 25°C 
                data.tempC = 25.0f; 
                data.tempValid = false;
            }
        }

        // Request อุณหภูมิรอบต่อไป (เพื่อเอาไว้อ่านใน Loop หน้า)
        ds.requestTemperatures();
        tempRequestPending = true;

        data.timestamp = millis();
        
        // Re-check ADS if it was not found initially (optional but safer)
        if (!data.adsValid) {
            data.adsValid = ads.begin();
        }

        if (data.adsValid) {
#if SENSOR_COUNT == 1
            int16_t rawAvg    = readADSAvg(ads, 0, 10);
            data.voltEC  = ads.computeVolts(rawAvg);

            data.valEC = SensorMath::calculateEC(
                data.voltEC,
                data.tempC,
                NVSManager::calibEC.alpha,
                NVSManager::calibEC.beta
            );
            data.valPPT = SensorMath::calculate(
                data.voltEC,
                data.tempC,
                NVSManager::calibEC.alpha,
                NVSManager::calibEC.beta
            );
#else
            // --- READ EC (A0) ---
            int16_t rawEC = readADSAvg(ads, 0, 10);
            data.voltEC   = ads.computeVolts(rawEC);
            data.valEC    = SensorMath::calculateEC(data.voltEC, data.tempC, NVSManager::calibEC.alpha, NVSManager::calibEC.beta);
            data.valPPT   = SensorMath::calculate(data.voltEC, data.tempC, NVSManager::calibEC.alpha, NVSManager::calibEC.beta);

            // --- READ pH (A1) ---
            int16_t rawPH = readADSAvg(ads, 1, 10);
            data.voltPH   = ads.computeVolts(rawPH);
            data.valPH    = SensorMath::calculatePH(data.voltPH, data.tempC, NVSManager::calibPH.alpha, NVSManager::calibPH.beta);

            // --- READ DO (A2) ---
            int16_t rawDO = readADSAvg(ads, 2, 10);
            data.voltDO   = ads.computeVolts(rawDO);
            data.valDO    = SensorMath::calculateDO(data.voltDO, data.tempC, NVSManager::calibDO.alpha, NVSManager::calibDO.beta);
#endif
        } else {
            // ADS missing -> set values to 0 or something distinctive
            data.valPPT = 0.0f;
            data.valEC = 0.0f;
#if SENSOR_COUNT == 3
            data.valPH = 0.0f;
            data.valDO = 0.0f;
#endif
        }


        xQueueOverwrite(sensorQueue, &data); // ส่งเข้า Queue ให้ UI นำไปโชว์

        // หน่วงเวลาของ Task
        vTaskDelay(pdMS_TO_TICKS(SENSOR_TASK_DELAY_MS));
    }
}