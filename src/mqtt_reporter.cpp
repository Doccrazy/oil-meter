#include "mqtt_reporter.h"

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <string.h>

#include <algorithm>

#include "config.h"
#include "log.h"
#include "secrets.h"

namespace mqtt {
namespace {

constexpr uint32_t kFlushDelayMs = 100;  // let lwIP send everything before disconnecting

struct Entity {
    const char* key;  // topic suffix and unique id suffix
    const char* name;
    const char* unit;          // nullptr = none
    const char* deviceClass;   // nullptr = none
    const char* icon;          // nullptr = none
    int precision;             // suggested display precision, -1 = none
    bool diagnostic;
};

constexpr Entity kEntities[] = {
    {"volume", "Oil volume", "L", "volume_storage", "mdi:barrel", 0, false},
    {"fill", "Fill level", "%", nullptr, "mdi:gauge", 0, false},
    {"free", "Free capacity", "L", "volume_storage", "mdi:barrel-outline", 0, false},
    {"height", "Oil height", "cm", "distance", nullptr, 1, false},
    {"distance", "Sensor distance", "cm", "distance", nullptr, 1, true},
    {"battery", "Battery", "%", "battery", nullptr, 0, true},
    {"battery_voltage", "Battery voltage", "V", "voltage", nullptr, 2, true},
    {"rssi", "WiFi signal", "dBm", "signal_strength", nullptr, 0, true},
    {"status", "Status", nullptr, nullptr, "mdi:information-outline", -1, true},
};

void stateTopic(char* buf, size_t len, const char* key) {
    snprintf(buf, len, "%s/%s/%s", cfg::MQTT_BASE_TOPIC, cfg::DEVICE_ID, key);
}

// Values expire in Home Assistant if the device stops reporting (dead battery etc.).
uint32_t expireAfterS() {
    return 2 * std::max(cfg::SLEEP_INTERVAL_S, cfg::LOW_BATTERY_SLEEP_INTERVAL_S) + 600;
}

bool publishDiscovery(PubSubClient& client) {
    char topic[128];
    char stateT[96];
    char payload[640];
    for (const Entity& e : kEntities) {
        stateTopic(stateT, sizeof(stateT), e.key);
        snprintf(topic, sizeof(topic), "%s/sensor/%s/%s/config", cfg::HA_DISCOVERY_PREFIX, cfg::DEVICE_ID, e.key);

        int n = snprintf(payload, sizeof(payload),
                         "{\"name\":\"%s\",\"uniq_id\":\"%s_%s\",\"obj_id\":\"%s_%s\",\"stat_t\":\"%s\","
                         "\"exp_aft\":%lu",
                         e.name, cfg::DEVICE_ID, e.key, cfg::DEVICE_ID, e.key, stateT,
                         static_cast<unsigned long>(expireAfterS()));
        auto append = [&](const char* fmt, auto... args) {
            if (n > 0 && static_cast<size_t>(n) < sizeof(payload)) {
                n += snprintf(payload + n, sizeof(payload) - n, fmt, args...);
            }
        };
        if (e.unit) {
            append(",\"unit_of_meas\":\"%s\",\"stat_cla\":\"measurement\"", e.unit);
        }
        if (e.deviceClass) append(",\"dev_cla\":\"%s\"", e.deviceClass);
        if (e.icon) append(",\"ic\":\"%s\"", e.icon);
        if (e.precision >= 0) append(",\"sug_dsp_prc\":%d", e.precision);
        if (e.diagnostic) append(",\"ent_cat\":\"diagnostic\"");
        append(",\"dev\":{\"ids\":[\"%s\"],\"name\":\"%s\",\"mf\":\"DIY\","
               "\"mdl\":\"FireBeetle 2 ESP32-C5 + JSN-SR04T\"}}",
               cfg::DEVICE_ID, cfg::DEVICE_NAME);

        if (n <= 0 || static_cast<size_t>(n) >= sizeof(payload)) {
            LOGF("Discovery payload for %s too long\n", e.key);
            return false;
        }
        if (!client.publish(topic, payload, true)) return false;
    }
    return true;
}

bool publishValue(PubSubClient& client, const char* key, float value, int decimals) {
    char topic[96];
    char payload[24];
    stateTopic(topic, sizeof(topic), key);
    snprintf(payload, sizeof(payload), "%.*f", decimals, value);
    LOGF("  %s = %s\n", topic, payload);
    return client.publish(topic, payload, true);
}

bool publishText(PubSubClient& client, const char* key, const char* value) {
    char topic[96];
    stateTopic(topic, sizeof(topic), key);
    LOGF("  %s = %s\n", topic, value);
    return client.publish(topic, value, true);
}

}  // namespace

bool publish(const Report& r, bool sendDiscovery) {
    WiFiClient net;
    PubSubClient client(net);
    client.setServer(MQTT_HOST, MQTT_PORT);
    client.setBufferSize(768);
    client.setSocketTimeout(cfg::MQTT_CONNECT_TIMEOUT_MS / 1000);

    const char* user = strlen(MQTT_USER) ? MQTT_USER : nullptr;
    const char* pass = strlen(MQTT_USER) ? MQTT_PASSWORD : nullptr;
    if (!client.connect(cfg::DEVICE_ID, user, pass)) {
        LOGF("MQTT connect failed, state %d\n", client.state());
        return false;
    }

    bool ok = true;
    if (sendDiscovery) {
        LOGF("Sending Home Assistant discovery\n");
        ok &= publishDiscovery(client);
    }

    if (r.levelOk) {
        ok &= publishValue(client, "volume", r.level.liters, 0);
        ok &= publishValue(client, "fill", r.level.percent, 1);
        ok &= publishValue(client, "free", r.level.capacityLiters - r.level.liters, 0);
        ok &= publishValue(client, "height", r.level.fillHeightCm, 1);
        ok &= publishValue(client, "distance", r.distanceCm, 1);
    }
    if (r.batteryOk) {
        ok &= publishValue(client, "battery", r.batteryPercent, 0);
        ok &= publishValue(client, "battery_voltage", r.batteryVolts, 2);
    }
    ok &= publishValue(client, "rssi", static_cast<float>(r.rssi), 0);
    ok &= publishText(client, "status", r.levelOk ? "ok" : "sensor_error");

    client.loop();
    delay(kFlushDelayMs);
    client.disconnect();
    net.stop();
    return ok;
}

}  // namespace mqtt
