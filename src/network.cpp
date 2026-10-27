#include "network.h"

#include <Arduino.h>
#include <WiFi.h>
#include <string.h>

#include "config.h"
#include "log.h"
#include "secrets.h"

namespace net {
namespace {

constexpr uint32_t kFastConnectTimeoutMs = 4000;

// Survives deep sleep. channel == 0 means "nothing cached".
RTC_DATA_ATTR uint8_t rtcBssid[6];
RTC_DATA_ATTR uint8_t rtcChannel = 0;

IPAddress toIp(const uint8_t (&a)[4]) {
    return IPAddress(a[0], a[1], a[2], a[3]);
}

bool waitConnected(uint32_t timeoutMs) {
    const uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > timeoutMs) return false;
        delay(10);
    }
    return true;
}

}  // namespace

bool connectWifi() {
    const uint32_t start = millis();

    if (cfg::USE_EXTERNAL_ANTENNA) digitalWrite(WIFI_ANT_CONFIG, HIGH);

    WiFi.persistent(false);  // don't wear flash with credentials on every wakeup
    WiFi.setHostname(cfg::DEVICE_ID);
    WiFi.mode(WIFI_STA);
    if (cfg::USE_STATIC_IP) {
        WiFi.config(toIp(cfg::STATIC_IP), toIp(cfg::STATIC_GATEWAY), toIp(cfg::STATIC_SUBNET), toIp(cfg::STATIC_DNS));
    }

    if (rtcChannel != 0) {
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD, rtcChannel, rtcBssid, true);
        if (waitConnected(kFastConnectTimeoutMs)) {
            LOGF("WiFi fast connect in %lu ms, RSSI %d\n", millis() - start, WiFi.RSSI());
            return true;
        }
        LOGF("WiFi fast connect failed, scanning\n");
        rtcChannel = 0;
        WiFi.disconnect();
    }

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    if (!waitConnected(cfg::WIFI_CONNECT_TIMEOUT_MS)) {
        LOGF("WiFi connect failed (status %d)\n", WiFi.status());
        return false;
    }
    memcpy(rtcBssid, WiFi.BSSID(), sizeof(rtcBssid));
    rtcChannel = WiFi.channel();
    LOGF("WiFi connected in %lu ms, channel %u, RSSI %d\n", millis() - start, rtcChannel, WiFi.RSSI());
    return true;
}

void shutdown() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
}

}  // namespace net
