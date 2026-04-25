#include "GPS.h"
#include "config.h"
#include <TinyGPS++.h>
#include <HardwareSerial.h>

void GPSTask::taskEntry(void* param) {
    HardwareSerial neogps(2);
    TinyGPSPlus    gps;

    neogps.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);

    GPSData data{};

    for (;;) {
        // รับ byte จาก GPS module ทั้งหมดที่มีในขณะนั้น
        while (neogps.available() > 0) {
            gps.encode(neogps.read());
        }

        // คัดลอกค่าจาก TinyGPS++ → struct
        data.valid      = gps.location.isValid();
        data.lat        = gps.location.isValid() ? gps.location.lat() : 0.0;
        data.lng        = gps.location.isValid() ? gps.location.lng() : 0.0;
        data.satellites = gps.satellites.isValid() ? gps.satellites.value() : 0;

        xQueueOverwrite(gpsQueue, &data);

        vTaskDelay(pdMS_TO_TICKS(GPS_TASK_DELAY_MS));
    }
}