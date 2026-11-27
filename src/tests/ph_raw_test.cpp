// ============================================================
// Test firmware: อ่านค่าดิบ pH (ADS1115 A2) + อุณหภูมิ (DS18B20)
// Build: pio run -e Test_pH_Raw -t upload && pio device monitor -b 115200
//
// - พิมพ์ CSV ทุก 1 วินาที เอาไปเปิดใน Excel ได้เลย
// - แสดงบน LCD ด้วย (ถ้าต่ออยู่)
// - ปิด SIM800L และ WiFi เพื่อลดสัญญาณรบกวน/กินไฟ
// - อ่าน alpha/beta ของ pH จาก NVS แบบ read-only (ไม่เขียนทับค่า Calibrate)
// ============================================================
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <Preferences.h>
#include <Adafruit_ADS1X15.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>
#include "SensorMath.h"

// ---- ตั้งค่า ----
#define PIN_SDA        21
#define PIN_SCL        22
#define PIN_ONEWIRE    18
#define PIN_MODEM_PWR  23
#define PH_CHANNEL     2      // pH ต่อที่ ADS1115 A2 (เหมือน firmware หลัก)
#define SAMPLES        10     // เฉลี่ยกี่ครั้งต่อ 1 รอบ (เหมือน firmware หลัก)
#define WINDOW         15     // หน้าต่างดูความนิ่ง (เหมือน Stability Tracker)
#define PERIOD_MS      1000
#define LCD_ADDR       0x27

// ไม่ใช้ queue ใน test นี้ แต่ Shared.h ประกาศ extern ไว้
QueueHandle_t sensorQueue = nullptr, gpsQueue = nullptr, inputQueue = nullptr;

Adafruit_ADS1115   ads;
OneWire            oneWire(PIN_ONEWIRE);
DallasTemperature  ds(&oneWire);
LiquidCrystal_I2C  lcd(LCD_ADDR, 20, 4);

static bool  adsOk = false, lcdOk = false;
static float phAlpha = 1.0f, phBeta = 0.0f, tempOffset = 0.0f;
static float voltWin[WINDOW];
static uint8_t winCount = 0, winHead = 0;

static bool i2cPresent(uint8_t addr) {
    Wire.beginTransmission(addr);
    return Wire.endTransmission() == 0;
}

static void loadCalib() {
    Preferences p;
    if (!p.begin("sys_data", true)) return;          // namespace ของรุ่น 3 หัว
    if (p.isKey("ph_a"))    phAlpha    = p.getFloat("ph_a", 1.0f);
    if (p.isKey("ph_b"))    phBeta     = p.getFloat("ph_b", 0.0f);
    if (p.isKey("tmp_off")) tempOffset = p.getFloat("tmp_off", 0.0f);
    p.end();
}

void setup() {
    Serial.begin(115200);

    // ปิด SIM800L + WiFi
    pinMode(PIN_MODEM_PWR, OUTPUT);
    digitalWrite(PIN_MODEM_PWR, LOW);
    WiFi.mode(WIFI_OFF);

    Wire.begin(PIN_SDA, PIN_SCL);
    adsOk = ads.begin();
    if (adsOk) ads.setGain(GAIN_ONE);                 // ±4.096 V, 0.125 mV/bit

    lcdOk = i2cPresent(LCD_ADDR);
    if (lcdOk) { lcd.init(); lcd.backlight(); lcd.clear(); }

    ds.begin();
    ds.setWaitForConversion(false);
    ds.requestTemperatures();

    loadCalib();

    delay(800);
    Serial.println();
    Serial.println("# pH raw test");
    Serial.printf("# ADS1115: %s | LCD: %s | DS18B20 count: %d\n",
                  adsOk ? "OK" : "NOT FOUND", lcdOk ? "OK" : "none", ds.getDeviceCount());
    Serial.printf("# NVS: ph_a=%.5f ph_b=%.5f tmp_off=%.2f\n", phAlpha, phBeta, tempOffset);
    Serial.println("t_s,raw,raw_min,raw_max,volt_V,temp_raw_C,temp_C,ph_base,ph_cal,win_n,win_range_mV");
}

void loop() {
    static uint32_t next = 0;
    if ((int32_t)(millis() - next) < 0) { delay(5); return; }
    next = millis() + PERIOD_MS;

    // --- อุณหภูมิ (อ่านผลของรอบที่แล้ว แล้วสั่งวัดรอบใหม่) ---
    float tRaw = ds.getTempCByIndex(0);
    bool  tOk  = (tRaw != DEVICE_DISCONNECTED_C && tRaw > -20.0f && tRaw < 80.0f);
    float tC   = tOk ? tRaw + tempOffset : 25.0f;     // fallback เหมือน firmware หลัก
    ds.requestTemperatures();

    if (!adsOk) {
        adsOk = ads.begin();
        if (adsOk) ads.setGain(GAIN_ONE);
        Serial.printf("%.1f,ADS_NOT_FOUND,,,,%.2f,%.2f\n", millis() / 1000.0f, tRaw, tC);
        return;
    }

    // --- pH: อ่าน 10 ครั้ง เฉลี่ย + เก็บ min/max ดูสัญญาณรบกวน ---
    long sum = 0; int16_t rMin = INT16_MAX, rMax = INT16_MIN;
    for (int i = 0; i < SAMPLES; i++) {
        int16_t r = ads.readADC_SingleEnded(PH_CHANNEL);
        sum += r;
        if (r < rMin) rMin = r;
        if (r > rMax) rMax = r;
        delay(5);
    }
    int16_t raw  = (int16_t)(sum / SAMPLES);
    float   volt = ads.computeVolts(raw);

    // --- คำนวณแบบเดียวกับ firmware หลัก ---
    float phBase = SensorMath::calculatePH(volt, tC, 1.0f, 0.0f);   // alpha=1, beta=0 = ค่าก่อน Calibrate
    float phCal  = SensorMath::calculatePH(volt, tC, phAlpha, phBeta);

    // --- หน้าต่างความนิ่ง (ช่วงแกว่งของแรงดันใน 15 วินาทีล่าสุด) ---
    voltWin[winHead] = volt;
    winHead = (winHead + 1) % WINDOW;
    if (winCount < WINDOW) winCount++;
    float vMin = voltWin[0], vMax = voltWin[0];
    for (uint8_t i = 1; i < winCount; i++) {
        if (voltWin[i] < vMin) vMin = voltWin[i];
        if (voltWin[i] > vMax) vMax = voltWin[i];
    }
    float rangeMv = (vMax - vMin) * 1000.0f;

    Serial.printf("%.1f,%d,%d,%d,%.5f,%.2f,%.2f,%.3f,%.3f,%u,%.2f\n",
                  millis() / 1000.0f, raw, rMin, rMax, volt,
                  tOk ? tRaw : NAN, tC, phBase, phCal, winCount, rangeMv);

    if (lcdOk) {
        char l[21];
        snprintf(l, sizeof(l), "RAW %6d V %.4f", raw, volt);          lcd.setCursor(0, 0); lcd.printf("%-20s", l);
        snprintf(l, sizeof(l), "T %5.2fC %s", tC, tOk ? "" : "(NO PROBE)"); lcd.setCursor(0, 1); lcd.printf("%-20s", l);
        snprintf(l, sizeof(l), "pHb %.2f pHc %.2f", phBase, phCal);   lcd.setCursor(0, 2); lcd.printf("%-20s", l);
        snprintf(l, sizeof(l), "n%2u rng %.2fmV", winCount, rangeMv); lcd.setCursor(0, 3); lcd.printf("%-20s", l);
    }
}
