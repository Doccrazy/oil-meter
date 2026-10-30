// Calibration firmware: streams raw sensor readings over USB serial (CDC) so a phone
// or PC can show them live. Everything stays powered, nothing sleeps, no WiFi.
//
// Protocol, one text line per event, 115200 8N1 (the baud rate is ignored by native USB):
//   H,oilmeter-cal,1        sent every 2 s and on boot: firmware id + protocol version
//   P,<seq>,<echo_us>       one ping; echo_us is the raw round trip time, 0 = no echo
//   B,<volts>               battery voltage every 2 s (< 2.5 V means no battery fitted)
//
// Distance, temperature compensation and offsets are computed by the app, so they can
// be tuned live without reflashing.

#include <Arduino.h>

#include "battery_monitor.h"
#include "config.h"
#include "sensor.h"

namespace {

constexpr uint32_t kStatusIntervalMs = 2000;

uint32_t seq = 0;
uint32_t lastStatusMs = 0;

void sendStatus() {
    Serial.println("H,oilmeter-cal,1");
    if (cfg::BATTERY_SENSE_ENABLED) Serial.printf("B,%.3f\n", battery::readVolts());
}

}  // namespace

void setup() {
    Serial.begin(115200);
    sensor::powerOn();
    sendStatus();
}

void loop() {
    const unsigned long us = sensor::pingEchoUs();
    Serial.printf("P,%lu,%lu\n", static_cast<unsigned long>(seq++), us);

    if (millis() - lastStatusMs >= kStatusIntervalMs) {
        lastStatusMs = millis();
        sendStatus();
    }
    delay(cfg::SENSOR_PING_INTERVAL_MS);
}
