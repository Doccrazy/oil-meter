#pragma once

// Copy this file to include/secrets.h and fill in your values.
// secrets.h is git-ignored.

#define WIFI_SSID "my-wifi"
#define WIFI_PASSWORD "my-wifi-password"

// Prefer an IP address over a hostname: saves a DNS lookup on every wakeup.
#define MQTT_HOST "192.168.1.10"
#define MQTT_PORT 1883
#define MQTT_USER ""      // leave empty for anonymous access
#define MQTT_PASSWORD ""
