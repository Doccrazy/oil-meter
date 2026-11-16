// WiFi + MQTT transport (see transport.h)

#include <WiFi.h>

#include "mqtt_reporter.h"
#include "network.h"
#include "transport.h"

namespace transport {

bool send(Report& report, bool /*timerWakeup*/, bool sendDiscovery) {
    bool sent = false;
    if (net::connectWifi()) {
        report.rssi = WiFi.RSSI();
        sent = mqtt::publish(report, sendDiscovery);
    }
    net::shutdown();
    return sent;
}

}  // namespace transport
