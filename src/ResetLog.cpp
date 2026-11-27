#include "ResetLog.h"
#include <Preferences.h>

namespace {
    const char* NS          = "rst_log";
    const char* KEY_BOOTS   = "boots";   // บูตทั้งหมด
    const char* KEY_ERRS    = "errs";    // รีเซตผิดปกติทั้งหมด
    const char* KEY_BROWN   = "brown";   // BROWNOUT (ไฟตก)
    const char* KEY_WDT     = "wdt";     // INT_WDT / TASK_WDT / WDT
    const char* KEY_PANIC   = "panic";   // PANIC / CPU_LOCKUP
    const char* KEY_LASTERR = "lasterr"; // สาเหตุผิดปกติครั้งล่าสุด (esp_reset_reason_t)

    esp_reset_reason_t s_reason = ESP_RST_UNKNOWN;
    uint32_t s_boots = 0, s_errs = 0, s_brown = 0, s_wdt = 0, s_panic = 0;
    uint8_t  s_lastErr = ESP_RST_UNKNOWN;

    const char* _name(esp_reset_reason_t r) {
        switch (r) {
            case ESP_RST_POWERON:    return "POWERON";   // เปิดไฟ / กด EN / เปิด Serial Monitor
            case ESP_RST_EXT:        return "EXT";
            case ESP_RST_SW:         return "SW";        // ESP.restart() เช่น หลัง OTA
            case ESP_RST_PANIC:      return "PANIC";     // crash (Guru Meditation)
            case ESP_RST_INT_WDT:    return "INT_WDT";   // CPU/flash ค้าง (มักมาจากไฟตก)
            case ESP_RST_TASK_WDT:   return "TASK_WDT";
            case ESP_RST_WDT:        return "WDT";
            case ESP_RST_DEEPSLEEP:  return "DEEPSLP";
            case ESP_RST_BROWNOUT:   return "BROWNOUT";  // ไฟ 3V3 ตก
            case ESP_RST_SDIO:       return "SDIO";
            case ESP_RST_PWR_GLITCH: return "GLITCH";
            case ESP_RST_CPU_LOCKUP: return "LOCKUP";
            default:                 return "UNKNOWN";
        }
    }

    bool _abnormal(esp_reset_reason_t r) {
        switch (r) {
            case ESP_RST_POWERON:
            case ESP_RST_EXT:
            case ESP_RST_SW:
            case ESP_RST_DEEPSLEEP:
                return false;
            default:
                return true;
        }
    }
}

namespace ResetLog {

void begin() {
    s_reason = esp_reset_reason();

    Preferences p;
    if (p.begin(NS, false)) {
        // getUInt/getUChar ไม่พิมพ์ log error เมื่อยังไม่มี key
        s_boots   = p.getUInt(KEY_BOOTS, 0)   + 1;
        s_errs    = p.getUInt(KEY_ERRS, 0);
        s_brown   = p.getUInt(KEY_BROWN, 0);
        s_wdt     = p.getUInt(KEY_WDT, 0);
        s_panic   = p.getUInt(KEY_PANIC, 0);
        s_lastErr = p.getUChar(KEY_LASTERR, ESP_RST_UNKNOWN);

        p.putUInt(KEY_BOOTS, s_boots);

        if (_abnormal(s_reason)) {
            p.putUInt(KEY_ERRS, ++s_errs);
            s_lastErr = (uint8_t)s_reason;
            p.putUChar(KEY_LASTERR, s_lastErr);

            if (s_reason == ESP_RST_BROWNOUT) {
                p.putUInt(KEY_BROWN, ++s_brown);
            } else if (s_reason == ESP_RST_INT_WDT || s_reason == ESP_RST_TASK_WDT || s_reason == ESP_RST_WDT) {
                p.putUInt(KEY_WDT, ++s_wdt);
            } else if (s_reason == ESP_RST_PANIC || s_reason == ESP_RST_CPU_LOCKUP) {
                p.putUInt(KEY_PANIC, ++s_panic);
            }
        }
        p.end();
    }

    Serial.printf("[RESET] reason=%s (%d)%s | boots=%lu errors=%lu (brownout=%lu wdt=%lu panic=%lu) last_error=%s\n",
                  _name(s_reason), (int)s_reason, _abnormal(s_reason) ? " <-- ABNORMAL" : "",
                  (unsigned long)s_boots, (unsigned long)s_errs,
                  (unsigned long)s_brown, (unsigned long)s_wdt, (unsigned long)s_panic,
                  _name((esp_reset_reason_t)s_lastErr));
}

esp_reset_reason_t reason()     { return s_reason; }
const char*        reasonName() { return _name(s_reason); }
bool               isAbnormal() { return _abnormal(s_reason); }
uint32_t           bootCount()  { return s_boots; }
uint32_t           errorCount() { return s_errs; }

} // namespace ResetLog
