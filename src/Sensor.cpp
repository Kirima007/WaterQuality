#include "Sensor.h"
#include "SalinityCalc.h"
#include "NVSManager.h"
#include "config.h"
#include <Adafruit_ADS1X15.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ==========================================
// FreeRTOS Task
// ==========================================
void SensorTask::taskEntry(void* param) {
    // --- Init Hardware ใน Task เลย ---
    // ADS1115 (ADC 16-bit สำหรับ conductivity probe)
    Adafruit_ADS1115 ads;
    ads.begin();
    ads.setGain(GAIN_ONE);  // ±4.096V range

    // DS18B20 (Temperature)
    OneWire oneWire(ONE_WIRE);
    DallasTemperature ds(&oneWire);
    ds.begin();
    ds.setWaitForConversion(false);  // non-blocking mode

    // --- ตัวแปรภายใน Task ---
    SensorData data{};
    bool tempReady = false;

    // Request อุณหภูมิครั้งแรก
    ds.requestTemperatures();

    // ==========================================
    // Main Loop
    // ==========================================
    for (;;) {
        // --- อ่านอุณหภูมิ (จาก request รอบที่แล้ว) ---
        if (tempReady) {
            float t = ds.getTempCByIndex(0);

            // ถ้าอ่านไม่ได้ (-127 = sensor error) ใช้ค่าเดิม
            if (t != DEVICE_DISCONNECTED_C) {
                data.currentTemp = t;
            }
        }

        // Request อุณหภูมิรอบต่อไป (จะพร้อมในรอบหน้า)
        ds.requestTemperatures();
        tempReady = true;

        // --- อ่านแรงดัน (ADS1115 channel 0) ---
        int16_t raw    = ads.readADC_SingleEnded(0);
        data.currentVolt   = ads.computeVolts(raw);

        // --- คำนวณความเค็ม ---
        // ดึงค่า calibration จาก NVSManager
        data.currentPPT = SalinityCalc::calculate(
            data.currentVolt,
            data.currentTemp,
            NVSManager::calib.v_di,
            NVSManager::calib.v_salt,
            NVSManager::calib.t_salt
        );

        // --- Timestamp ---
        data.timestamp = millis();

        // --- ส่งเข้า Queue (ทับค่าเก่า เอาแค่ค่าล่าสุด) ---
        xQueueOverwrite(sensorQueue, &data);

        vTaskDelay(pdMS_TO_TICKS(SENSOR_TASK_DELAY_MS));
    }
}