// Zigbee transport (see transport.h)
//
// The board is a battery powered Zigbee end device. Every value lives on its own
// endpoint as an Analog Input cluster (with description and BACnet engineering unit),
// the sensor status is a Binary Input, the battery charge is the standard Power
// Configuration cluster. Reports are sent straight to the coordinator, so no binding
// is needed; their APS acknowledgements tell whether the report arrived.
//
//   EP 10  volume           L   (+ battery percentage, Power Configuration cluster)
//   EP 11  fill             %
//   EP 12  free             L
//   EP 13  height           cm
//   EP 14  distance         cm
//   EP 15  battery_voltage  V
//   EP 16  sensor_error     Binary Input, true = sensor error
//
// Pairing: enable "permit join" on the coordinator, press RST. To move to another
// network press RST, then BOOT while the LED is lit.

#include <Arduino.h>

#ifndef ZIGBEE_MODE_ED
#error "Zigbee end device mode not enabled, build with a firebeetle2_c5_zigbee* environment"
#endif

#include <Zigbee.h>
#include <zcl/esp_zigbee_zcl_power_config.h>

#include <atomic>

#include "config.h"
#include "log.h"
#include "transport.h"

namespace transport {
namespace {

// BACnet engineering units (Analog Input cluster, EngineeringUnits attribute)
constexpr uint16_t kUnitVolts = 5;
constexpr uint16_t kUnitLiters = 82;
constexpr uint16_t kUnitPercent = 98;
constexpr uint16_t kUnitCentimeters = 118;

constexpr uint16_t kCoordinatorShortAddr = 0x0000;
constexpr uint8_t kCoordinatorEndpoint = 1;

// Reports sent but not yet acknowledged / failed, updated from the Zigbee task.
std::atomic<int> pendingReports{0};
std::atomic<int> failedReports{0};

void onSendStatus(esp_zb_zcl_command_send_status_message_t message) {
    if (message.status != ESP_OK) ++failedReports;
    --pendingReports;
}

// Adds unicast reporting to the coordinator to a library endpoint class.
template <class Base>
class Endpoint : public Base {
public:
    explicit Endpoint(uint8_t endpoint) : Base(endpoint) {}

    bool reportToCoordinator(uint16_t cluster, uint16_t attribute) {
        esp_zb_zcl_report_attr_cmd_t cmd{};
        cmd.zcl_basic_cmd.dst_addr_u.addr_short = kCoordinatorShortAddr;
        cmd.zcl_basic_cmd.dst_endpoint = kCoordinatorEndpoint;
        cmd.zcl_basic_cmd.src_endpoint = this->getEndpoint();
        cmd.address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
        cmd.clusterID = cluster;
        cmd.direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_CLI;
        cmd.dis_default_resp = 1;  // the APS ack is enough, saves a round trip
        cmd.manuf_code = ESP_ZB_ZCL_ATTR_NON_MANUFACTURER_SPECIFIC;
        cmd.attributeID = attribute;

        ++pendingReports;
        if (!this->reportClusterAttribute(&cmd)) {
            --pendingReports;
            ++failedReports;
            return false;
        }
        return true;
    }

    bool reportBattery() {
        return reportToCoordinator(ESP_ZB_ZCL_CLUSTER_ID_POWER_CONFIG,
                                   ESP_ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_PERCENTAGE_REMAINING_ID);
    }
};

class AnalogSensor : public Endpoint<ZigbeeAnalog> {
public:
    AnalogSensor(uint8_t endpoint, const char* description, uint16_t unit, float resolution)
        : Endpoint(endpoint), description_(description), unit_(unit), resolution_(resolution) {}

    // Create the clusters, before Zigbee.addEndpoint().
    void setup() {
        setManufacturerAndModel(cfg::ZIGBEE_MANUFACTURER, cfg::ZIGBEE_MODEL);
        addAnalogInput();
        setAnalogInputDescription(description_);
        setAnalogInputResolution(resolution_);
        esp_zb_attribute_list_t* cluster = esp_zb_cluster_list_get_cluster(
            _cluster_list, ESP_ZB_ZCL_CLUSTER_ID_ANALOG_INPUT, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
        uint16_t unit = unit_;
        if (cluster == nullptr ||
            esp_zb_analog_input_cluster_add_attr(cluster, ESP_ZB_ZCL_ATTR_ANALOG_INPUT_ENGINEERING_UNITS_ID, &unit) !=
                ESP_OK) {
            LOGF("Zigbee: cannot set unit of EP %u\n", getEndpoint());
        }
    }

    bool report(float value) {
        return setAnalogInput(value) &&
               reportToCoordinator(ESP_ZB_ZCL_CLUSTER_ID_ANALOG_INPUT, ESP_ZB_ZCL_ATTR_ANALOG_INPUT_PRESENT_VALUE_ID);
    }

private:
    const char* description_;
    uint16_t unit_;
    float resolution_;
};

class BinarySensor : public Endpoint<ZigbeeBinary> {
public:
    BinarySensor(uint8_t endpoint, const char* description) : Endpoint(endpoint), description_(description) {}

    void setup() {
        setManufacturerAndModel(cfg::ZIGBEE_MANUFACTURER, cfg::ZIGBEE_MODEL);
        addBinaryInput();
        setBinaryInputDescription(description_);
    }

    bool report(bool value) {
        return setBinaryInput(value) &&
               reportToCoordinator(ESP_ZB_ZCL_CLUSTER_ID_BINARY_INPUT, ESP_ZB_ZCL_ATTR_BINARY_INPUT_PRESENT_VALUE_ID);
    }

private:
    const char* description_;
};

AnalogSensor epVolume(10, "Oil volume", kUnitLiters, 1.0f);
AnalogSensor epFill(11, "Fill level", kUnitPercent, 0.1f);
AnalogSensor epFree(12, "Free capacity", kUnitLiters, 1.0f);
AnalogSensor epHeight(13, "Oil height", kUnitCentimeters, 0.1f);
AnalogSensor epDistance(14, "Sensor distance", kUnitCentimeters, 0.1f);
AnalogSensor epBatteryVoltage(15, "Battery voltage", kUnitVolts, 0.01f);
BinarySensor epSensorError(16, "Sensor error");

AnalogSensor* const kAnalogEndpoints[] = {&epVolume, &epFill,     &epFree,
                                          &epHeight, &epDistance, &epBatteryVoltage};

void setLed(bool on) {
    digitalWrite(cfg::PIN_LED, on ? HIGH : LOW);
}

// LED on for ZIGBEE_LEAVE_WINDOW_MS; true if BOOT was pressed meanwhile.
bool leaveRequested() {
    pinMode(cfg::PIN_BOOT_BUTTON, INPUT_PULLUP);
    setLed(true);
    const uint32_t start = millis();
    while (millis() - start < cfg::ZIGBEE_LEAVE_WINDOW_MS) {
        if (digitalRead(cfg::PIN_BOOT_BUTTON) == LOW) {
            for (int i = 0; i < 6; ++i) {  // acknowledge with fast blinking
                setLed(i % 2);
                delay(100);
            }
            return true;
        }
        delay(20);
    }
    setLed(false);
    return false;
}

template <class Predicate>
bool waitFor(Predicate done, uint32_t timeoutMs) {
    const uint32_t start = millis();
    while (!done()) {
        if (millis() - start > timeoutMs) return false;
        delay(10);
    }
    return true;
}

}  // namespace

bool send(Report& r, bool timerWakeup, bool /*sendDiscovery*/) {
    const uint32_t start = millis();
    pinMode(cfg::PIN_LED, OUTPUT);

    const bool leave = !timerWakeup && leaveRequested();
    if (leave) LOGF("Zigbee: forgetting the network, joining a new one\n");

    // Clusters and endpoints have to exist before the stack starts.
    for (AnalogSensor* ep : kAnalogEndpoints) ep->setup();
    epSensorError.setup();
    if (cfg::BATTERY_SENSE_ENABLED) {
        const uint8_t percent = r.batteryOk ? static_cast<uint8_t>(r.batteryPercent + 0.5f) : 0xff;
        const uint8_t decivolts = r.batteryOk ? static_cast<uint8_t>(r.batteryVolts * 10.0f + 0.5f) : 0xff;
        epVolume.setPowerSource(ZB_POWER_SOURCE_BATTERY, percent, decivolts);
    }
    for (AnalogSensor* ep : kAnalogEndpoints) Zigbee.addEndpoint(ep);
    Zigbee.addEndpoint(&epSensorError);

    // Long child timeout on the parent, so it still knows us after a 24 h sleep and
    // the wakeup is a quick rejoin instead of a full association.
    esp_zb_cfg_t zbConfig = ZIGBEE_DEFAULT_ED_CONFIG();
    zbConfig.nwk_cfg.zed_cfg.ed_timeout = ESP_ZB_ED_AGING_TIMEOUT_16384MIN;
    zbConfig.nwk_cfg.zed_cfg.keep_alive = 10000;

    const uint32_t joinTimeoutMs = timerWakeup ? cfg::ZIGBEE_CONNECT_TIMEOUT_MS : cfg::ZIGBEE_PAIRING_TIMEOUT_MS;
    Zigbee.setTimeout(joinTimeoutMs);
    setLed(!timerWakeup);  // LED on while pairing after a reset
    if (!Zigbee.begin(&zbConfig, leave)) {
        LOGF("Zigbee: stack start / rejoin failed\n");
        setLed(false);
        return false;
    }
    if (!waitFor([] { return Zigbee.connected(); }, joinTimeoutMs)) {
        LOGF("Zigbee: no network joined (permit join enabled on the coordinator?)\n");
        setLed(false);
        return false;
    }
    setLed(false);
    LOGF("Zigbee: connected in %lu ms\n", millis() - start);

    if (esp_zb_lock_acquire(portMAX_DELAY)) {
        esp_zb_zcl_command_send_status_handler_register(onSendStatus);
        esp_zb_lock_release();
    }

    if (r.levelOk) {
        epVolume.report(r.level.liters);
        epFill.report(r.level.percent);
        epFree.report(r.level.capacityLiters - r.level.liters);
        epHeight.report(r.level.fillHeightCm);
        epDistance.report(r.distanceCm);
    }
    epSensorError.report(!r.levelOk);
    if (r.batteryOk) {
        epVolume.reportBattery();
        epBatteryVoltage.report(r.batteryVolts);
    }

    const bool acked = waitFor([] { return pendingReports.load() <= 0; }, cfg::ZIGBEE_REPORT_TIMEOUT_MS);
    const int failed = failedReports.load();
    LOGF("Zigbee: reports %s, %d failed, %d unacknowledged\n", acked && failed == 0 ? "ok" : "incomplete", failed,
         pendingReports.load());

    if (!timerWakeup) {
        LOGF("Zigbee: staying awake %lu s for the coordinator interview\n", cfg::ZIGBEE_INTERVIEW_TIME_MS / 1000);
        delay(cfg::ZIGBEE_INTERVIEW_TIME_MS);
    }
    return acked && failed == 0;
}

}  // namespace transport
