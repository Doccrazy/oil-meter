#pragma once

// =====================================================================================
//  Oil level monitor – user configuration
//  WiFi / MQTT credentials live in secrets.h (copy secrets.example.h), not needed
//  for the Zigbee build.
// =====================================================================================

#include <stdint.h>

#include "tank_geometry.h"

#ifndef OILMETER_DEBUG
#define OILMETER_DEBUG 0
#endif

namespace cfg {

// -------------------------------------------------------------------------------------
//  Tank geometry  (all inner dimensions, in cm)
// -------------------------------------------------------------------------------------
//  Shape::Rectangular        -> TANK_LENGTH_CM, TANK_WIDTH_CM, TANK_HEIGHT_CM
//  Shape::VerticalCylinder   -> TANK_DIAMETER_CM, TANK_HEIGHT_CM
//  Shape::HorizontalCylinder -> TANK_DIAMETER_CM, TANK_LENGTH_CM
//  Shape::Table              -> TANK_TABLE (height -> liters of ONE tank, e.g. from
//                               the manufacturer's "Peiltabelle"), most accurate
//                               for rounded plastic tanks.
constexpr tank::Shape TANK_SHAPE = tank::Shape::Table;

constexpr float TANK_HEIGHT_CM = 145.0f;
constexpr float TANK_LENGTH_CM = 212.0f;
constexpr float TANK_WIDTH_CM = 65.0f;
constexpr float TANK_DIAMETER_CM = 100.0f;

// Number of identical tanks connected at the bottom (battery tank system). The sensor
// sits in one of them; the measured volume is multiplied by this count.
constexpr unsigned TANK_COUNT = 2;

// Distance from the sensor face down to the highest possible oil level (inner tank
// top). The JSN-SR04T cannot measure closer than ~25 cm, so if the tank can be filled
// to the top, mount the sensor on a riser so that this offset is >= 25 cm.
constexpr float SENSOR_OFFSET_CM = 6.0f;

// Calibration table, only used with Shape::Table. Ascending height, liters per tank.
constexpr tank::TablePoint TANK_TABLE[] = {
    // Nau-Nylon-BAF 2000 L
    {0.0f, 0.0f},
    {9.0f, 100.0f},
    {15.8f, 200.0f},
    {22.5f, 300.0f},
    {29.2f, 400.0f},
    {35.9f, 500.0f},
    {42.6f, 600.0f},
    {49.3f, 700.0f},
    {54.0f, 800.0f},
    {62.7f, 900.0f},
    {69.4f, 1000.0f},
    {76.1f, 1100.0f},
    {82.8f, 1200.0f},
    {89.5f, 1300.0f},
    {96.2f, 1400.0f},
    {102.9f, 1500.0f},
    {109.6f, 1600.0f},
    {116.0f, 1700.0f},
    {122.6f, 1800.0f},
    {130.0f, 1900.0f},
    {139.0f, 2000.0f},
};

constexpr tank::Config TANK = {
    TANK_SHAPE,
    TANK_HEIGHT_CM,
    TANK_LENGTH_CM,
    TANK_WIDTH_CM,
    TANK_DIAMETER_CM,
    TANK_COUNT,
    TANK_TABLE,
    sizeof(TANK_TABLE) / sizeof(TANK_TABLE[0]),
    SENSOR_OFFSET_CM,
};

// -------------------------------------------------------------------------------------
//  Ultrasonic measurement (JSN-SR04T in mode 1 = trigger/echo)
// -------------------------------------------------------------------------------------
// Air temperature inside the tank, used for the speed of sound (~0.18 %/°C).
// A basement tank is usually around 10-15 °C.
constexpr float AIR_TEMPERATURE_C = 12.0f;
constexpr int SENSOR_SAMPLES = 9;            // median of N pings
constexpr int SENSOR_MIN_VALID_SAMPLES = 5;  // otherwise reported as sensor error
constexpr uint32_t SENSOR_WARMUP_MS = 500;   // after switching sensor power on
constexpr uint32_t SENSOR_PING_INTERVAL_MS = 70;
constexpr float SENSOR_MIN_DISTANCE_CM = 20.0f;   // blind zone of the JSN-SR04T
constexpr float SENSOR_MAX_DISTANCE_CM = 450.0f;

// -------------------------------------------------------------------------------------
//  Timing
// -------------------------------------------------------------------------------------
#if OILMETER_DEBUG
constexpr uint32_t SLEEP_INTERVAL_S = 5;
#else
constexpr uint32_t SLEEP_INTERVAL_S = 6UL * 3600UL;  // normal measurement interval
#endif
constexpr uint32_t LOW_BATTERY_SLEEP_INTERVAL_S = 24UL * 3600UL;
constexpr uint32_t RETRY_INTERVAL_S = 15UL * 60UL;  // first retry after WiFi/MQTT failure
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 10000;
constexpr uint32_t MQTT_CONNECT_TIMEOUT_MS = 5000;

// -------------------------------------------------------------------------------------
//  Battery (single cell Li-ion / LiPo on the FireBeetle BAT connector)
// -------------------------------------------------------------------------------------
constexpr bool BATTERY_SENSE_ENABLED = true;
// On-board voltage divider BAT+ -> R_TOP -> IO1 -> R_BOTTOM -> GND. DFRobot does not
// document the values; FireBeetle 2 boards use a 1:1 divider (battery = 2 x pin).
// Only the ratio matters. Check it once against a multimeter (BATTERY_CALIBRATION).
constexpr float BATTERY_DIVIDER_R_TOP = 1.0f;
constexpr float BATTERY_DIVIDER_R_BOTTOM = 1.0f;
// Multiply the measured voltage by this to match a multimeter reading.
constexpr float BATTERY_CALIBRATION = 1.0f;
constexpr float BATTERY_LOW_V = 3.55f;       // -> longer sleep interval
constexpr float BATTERY_CRITICAL_V = 3.35f;  // -> no WiFi at all, just sleep

// -------------------------------------------------------------------------------------
//  Network
// -------------------------------------------------------------------------------------
constexpr const char* DEVICE_ID = "oilmeter";      // MQTT client id + topic prefix
constexpr const char* DEVICE_NAME = "Oil tank";    // name shown in Home Assistant
constexpr const char* MQTT_BASE_TOPIC = "oilmeter";  // -> oilmeter/<DEVICE_ID>/...

// Home Assistant MQTT auto discovery
constexpr bool HA_DISCOVERY_ENABLED = true;
constexpr const char* HA_DISCOVERY_PREFIX = "homeassistant";
// Discovery is re-sent after every reset and every N timer wakeups.
constexpr uint32_t HA_DISCOVERY_EVERY_N_WAKEUPS = 28;

// A static IP saves the DHCP round trip (~0.3-1 s of radio time per wakeup).
constexpr bool USE_STATIC_IP = false;
constexpr uint8_t STATIC_IP[4] = {192, 168, 1, 50};
constexpr uint8_t STATIC_GATEWAY[4] = {192, 168, 1, 1};
constexpr uint8_t STATIC_SUBNET[4] = {255, 255, 255, 0};
constexpr uint8_t STATIC_DNS[4] = {192, 168, 1, 1};

// -------------------------------------------------------------------------------------
//  Zigbee  (only used by the firebeetle2_c5_zigbee* build environments)
// -------------------------------------------------------------------------------------
// The board joins an existing Zigbee network (Zigbee2MQTT, ZHA, deCONZ, ...) as a
// battery powered end device. Manufacturer / model as shown by the coordinator; the
// Zigbee2MQTT converter in zigbee2mqtt/ matches on ZIGBEE_MODEL.
constexpr const char* ZIGBEE_MANUFACTURER = "DIY";
constexpr const char* ZIGBEE_MODEL = "OilMeter";
// Rejoin + report on a timer wakeup, before giving up (-> retry with backoff).
constexpr uint32_t ZIGBEE_CONNECT_TIMEOUT_MS = 10000;
// After a reset / power on: how long to wait for joining. Enable "permit join" on the
// coordinator, then press RST.
constexpr uint32_t ZIGBEE_PAIRING_TIMEOUT_MS = 180000;
// After a reset: stay awake this long once joined, so the coordinator can interview
// the device (read its endpoints and clusters).
constexpr uint32_t ZIGBEE_INTERVIEW_TIME_MS = 30000;
// Wait this long for the coordinator to acknowledge all reports.
constexpr uint32_t ZIGBEE_REPORT_TIMEOUT_MS = 5000;
// After a reset the LED is lit this long. Pressing BOOT meanwhile forgets the current
// network and joins a new one (e.g. a different coordinator).
constexpr uint32_t ZIGBEE_LEAVE_WINDOW_MS = 3000;

// -------------------------------------------------------------------------------------
//  Pins (DFRobot FireBeetle 2 ESP32-C5)
// -------------------------------------------------------------------------------------
constexpr int PIN_BATTERY_ADC = 1;    // IO1, on-board battery divider
constexpr int PIN_SENSOR_POWER = 0;   // IO0, enables the switched 3V3_C output, HIGH = on
constexpr int PIN_SENSOR_TRIG = 8;    // 8/D2
constexpr int PIN_SENSOR_ECHO = 26;   // 26/D3
constexpr int PIN_BOOT_BUTTON = 28;   // on-board BOOT button, LOW = pressed
constexpr int PIN_LED = 15;           // on-board LED, HIGH = on

}  // namespace cfg
