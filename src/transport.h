#pragma once

#include "tank_geometry.h"

// Delivery of one measurement. The transport is chosen at build time:
//   transport_wifi.cpp   -> WiFi + MQTT (Home Assistant discovery)   pio env firebeetle2_c5*
//   transport_zigbee.cpp -> Zigbee end device                        pio env firebeetle2_c5_zigbee*
namespace transport {

struct Report {
    bool levelOk;
    tank::Level level;
    float distanceCm;
    bool batteryOk;
    float batteryVolts;
    float batteryPercent;
    int rssi;  // filled in by the WiFi transport
};

// Switch the radio on, deliver the report, switch the radio off again.
// timerWakeup:   false after a reset / power on (Zigbee: pairing window)
// sendDiscovery: (re)send Home Assistant MQTT discovery (ignored by Zigbee)
bool send(Report& report, bool timerWakeup, bool sendDiscovery);

}  // namespace transport
