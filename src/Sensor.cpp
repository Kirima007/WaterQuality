#include "Sensor.h"
#include "SalinityCalc.h"
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
    if (!ads.begin()) {
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
    data.currentTemp = 25.0f; // ตั้งค่าเริ่มต้นเป็น 25 องศา เผื่อเซ็นเซอร์อ่านไม่ได้
    bool tempReady = false;

    // Request อุณหภูมิครั้งแรก
    ds.requestTemperatures();

    // ==========================================
    // Main Loop
    // ==========================================
    for (;;) {
        // --- 1. จัดการอุณหภูมิ ---
        if (tempReady) {
            float t = ds.getTempCByIndex(0);

            // เช็คว่าอ่านค่าได้จริง
            if (t != DEVICE_DISCONNECTED_C && t > -20.0f && t < 80.0f) {
                data.currentTemp = t;
            } else {
                // Fallback: ถ้าระบบหาเซ็นเซอร์อุณหภูมิไม่เจอ ให้ล็อกค่าไว้ที่ 25°C 
                data.currentTemp = 25.0f; 
            }
        }

        // Request อุณหภูมิรอบต่อไป (เพื่อเอาไว้อ่านใน Loop หน้า)
        ds.requestTemperatures();
        tempReady = true;

        int16_t rawAvg    = readADSAvg(ads, 0, 10);
        data.currentVolt  = ads.computeVolts(rawAvg);

        // ดึงค่า calibration จาก NVSManager มาเข้าสมการของ SalinityCalc

        data.currentEC = SalinityCalc::calculateEC(
            data.currentVolt,
            data.currentTemp,
            NVSManager::calib.alpha,
            NVSManager::calib.beta
        );
        data.currentPPT = SalinityCalc::calculate(
            data.currentVolt,
            data.currentTemp,
            NVSManager::calib.alpha,
            NVSManager::calib.beta
        );

        data.timestamp = millis();
        xQueueOverwrite(sensorQueue, &data); // ส่งเข้า Queue ให้ UI นำไปโชว์

        // หน่วงเวลาของ Task
        vTaskDelay(pdMS_TO_TICKS(SENSOR_TASK_DELAY_MS));
    }
}