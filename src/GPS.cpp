#include "GPS.h"
#include "config.h"
#include <TinyGPS++.h>
#include <HardwareSerial.h>

void GPSTask::taskEntry(void* param) {
    HardwareSerial neogps(2);
    TinyGPSPlus    gps;

    neogps.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);

    GPSData data{};
    uint32_t lastCharTime = millis();

    for (;;) {
        // รับ byte จาก GPS module ทั้งหมดที่มีในขณะนั้น
        bool charReceived = false;
        while (neogps.available() > 0) {
            gps.encode(neogps.read());
            charReceived = true;
            lastCharTime = millis();
        }

        // เช็ค Serial Error (ถ้าไม่มี data นานเกิน 5 วินาที)
        data.serialError = (millis() - lastCharTime > 5000);

        // คัดลอกค่าจาก TinyGPS++ → struct
        data.valid      = gps.location.isValid();
        data.lat        = gps.location.isValid() ? gps.location.lat() : 0.0;
        data.lng        = gps.location.isValid() ? gps.location.lng() : 0.0;
        data.satellites = gps.satellites.isValid() ? gps.satellites.value() : 0;

        xQueueOverwrite(gpsQueue, &data);

        vTaskDelay(pdMS_TO_TICKS(GPS_TASK_DELAY_MS));
    }
}