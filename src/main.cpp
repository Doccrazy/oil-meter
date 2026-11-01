// Battery powered oil level monitor
// DFRobot FireBeetle 2 ESP32-C5 + JSN-SR04T ultrasonic sensor (on the switched 3V3_C output)
//
// Every wakeup: measure battery -> measure distance -> WiFi -> MQTT -> deep sleep.
// The radio is only switched on after the measurement, and stays on for ~1-2 s.

#include <Arduino.h>
#include <WiFi.h>
#include <esp_sleep.h>

#include <algorithm>

#include "battery_curve.h"
#include "battery_monitor.h"
#include "config.h"
#include "log.h"
#include "mqtt_reporter.h"
#include "network.h"
#include "sensor.h"
#include "tank_geometry.h"

namespace {

// Survive deep sleep, reset on power loss.
RTC_DATA_ATTR uint32_t rtcWakeups = 0;
RTC_DATA_ATTR uint32_t rtcFailures = 0;
RTC_DATA_ATTR uint32_t rtcWakeupsSinceDiscovery = 0;

// Below this the ADC sees no battery (USB powered without a cell).
constexpr float kNoBatteryVolts = 2.5f;

[[noreturn]] void deepSleep(uint32_t seconds) {
    LOGF("Awake for %lu ms, sleeping %lu s\n", millis(), static_cast<unsigned long>(seconds));
#if OILMETER_DEBUG
    Serial.flush();
#endif
    sensor::powerOff();
    esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(seconds) * 1000000ULL);
    esp_deep_sleep_start();
}

// Exponential backoff after failed reports, capped at the regular interval.
uint32_t retryIntervalS(uint32_t failures, uint32_t regularS) {
    const uint32_t shift = std::min<uint32_t>(failures > 0 ? failures - 1 : 0, 8);
    return std::min<uint32_t>(cfg::RETRY_INTERVAL_S << shift, regularS);
}

}  // namespace

void setup() {
    setCpuFrequencyMhz(80);  // lowest clock that still supports WiFi
    sensor::powerOff();

#if OILMETER_DEBUG
    Serial.begin(115200);
    delay(2000);  // give the USB CDC host time to attach
#endif

    const bool timerWakeup = esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER;
    if (!timerWakeup) {
        rtcFailures = 0;
        rtcWakeupsSinceDiscovery = 0;
    }
    ++rtcWakeups;
    LOGF("\n=== Oil meter wakeup #%lu (%s) ===\n", static_cast<unsigned long>(rtcWakeups),
         timerWakeup ? "timer" : "reset");

    mqtt::Report report{};

    // Battery first, while nothing else draws current.
    if (cfg::BATTERY_SENSE_ENABLED) {
        report.batteryVolts = battery::readVolts();
        report.batteryOk = report.batteryVolts >= kNoBatteryVolts;
        report.batteryPercent = battery::lipoPercent(report.batteryVolts);
        LOGF("Battery: %.2f V (%.0f %%)%s\n", report.batteryVolts, report.batteryPercent,
             report.batteryOk ? "" : " -> no battery detected, ignoring");
    }

    if (report.batteryOk && report.batteryVolts < cfg::BATTERY_CRITICAL_V) {
        LOGF("Battery critical, skipping measurement and WiFi\n");
        deepSleep(cfg::LOW_BATTERY_SLEEP_INTERVAL_S);
    }
    const bool lowBattery = report.batteryOk && report.batteryVolts < cfg::BATTERY_LOW_V;
    const uint32_t regularIntervalS = lowBattery ? cfg::LOW_BATTERY_SLEEP_INTERVAL_S : cfg::SLEEP_INTERVAL_S;

    LOGF("Measuring...\n");
    const sensor::Reading reading = sensor::measure();
    report.levelOk = reading.ok;
    report.distanceCm = reading.distanceCm;
    if (reading.ok) {
        report.level = tank::levelFromDistance(cfg::TANK, reading.distanceCm);
        LOGF("Distance %.1f cm (%d valid) -> height %.1f cm, %.0f / %.0f L (%.1f %%)\n", reading.distanceCm,
             reading.validSamples, report.level.fillHeightCm, report.level.liters, report.level.capacityLiters,
             report.level.percent);
    } else {
        LOGF("Sensor error: only %d valid pings\n", reading.validSamples);
    }

    const bool sendDiscovery = cfg::HA_DISCOVERY_ENABLED &&
                               (!timerWakeup || rtcWakeupsSinceDiscovery >= cfg::HA_DISCOVERY_EVERY_N_WAKEUPS);

    bool sent = false;
    if (net::connectWifi()) {
        report.rssi = WiFi.RSSI();
        sent = mqtt::publish(report, sendDiscovery);
    }
    net::shutdown();

    if (sent) {
        rtcFailures = 0;
        rtcWakeupsSinceDiscovery = sendDiscovery ? 0 : rtcWakeupsSinceDiscovery + 1;
        deepSleep(regularIntervalS);
    }

    ++rtcFailures;
    LOGF("Report failed (%lu in a row)\n", static_cast<unsigned long>(rtcFailures));
    deepSleep(retryIntervalS(rtcFailures, regularIntervalS));
}

void loop() {
    // Never reached: setup() always ends in deep sleep.
}
