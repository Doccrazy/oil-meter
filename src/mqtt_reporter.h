#pragma once

#include "transport.h"

namespace mqtt {

// Connect to the broker, optionally (re)send Home Assistant discovery, publish all
// values retained under <MQTT_BASE_TOPIC>/<DEVICE_ID>/<key>, then disconnect.
bool publish(const transport::Report& report, bool sendDiscovery);

}  // namespace mqtt
