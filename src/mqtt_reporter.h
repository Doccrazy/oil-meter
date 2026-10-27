#pragma once

#include "tank_geometry.h"

namespace mqtt {

struct Report {
    bool levelOk;
    tank::Level level;
    float distanceCm;
    bool batteryOk;
    float batteryVolts;
    float batteryPercent;
    int rssi;
};

// Connect to the broker, optionally (re)send Home Assistant discovery, publish all
// values retained under <MQTT_BASE_TOPIC>/<DEVICE_ID>/<key>, then disconnect.
bool publish(const Report& report, bool sendDiscovery);

}  // namespace mqtt
