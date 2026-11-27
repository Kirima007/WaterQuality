#pragma once
#include <Arduino.h>
#include <esp_system.h>

// ==========================================
// ResetLog — บันทึกสาเหตุการรีเซต + นับจำนวนครั้งลง NVS
//
// - เรียก begin() ครั้งเดียวตอนต้น setup() (ก่อนสร้าง Task)
// - ใช้ namespace แยก "rst_log" → Factory Reset (NVSManager::reset) ไม่ลบประวัตินี้
// - ค่าทั้งหมดถูกอ่านเก็บไว้ใน RAM ตอน begin() → เรียก getter จาก Task ไหนก็ได้
// ==========================================
namespace ResetLog {
    void               begin();
    esp_reset_reason_t reason();       // สาเหตุการบูตรอบนี้
    const char*        reasonName();   // ชื่อย่อไม่เกิน 8 ตัว เช่น "BROWNOUT" (ใช้บน LCD / JSON)
    bool               isAbnormal();   // รอบนี้รีเซตเพราะความผิดปกติไหม (PANIC/WDT/BROWNOUT/...)
    uint32_t           bootCount();    // บูตทั้งหมดกี่ครั้ง
    uint32_t           errorCount();   // รีเซตผิดปกติสะสมกี่ครั้ง
}
