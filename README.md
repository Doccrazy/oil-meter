# Oil level monitor

Battery powered heating oil level monitor based on a DFRobot FireBeetle 2 ESP32-C5 and a
JSN-SR04T waterproof ultrasonic sensor. It wakes up periodically, measures the
distance to the oil surface, converts it to liters and publishes the values via MQTT
(with Home Assistant auto discovery) or, alternatively, [via Zigbee](#zigbee-instead-of-wifi),
then goes back to deep sleep.

* Wiring, schematic, mounting and calibration: [docs/WIRING.md](docs/WIRING.md)
* All tank dimensions and tuning parameters: [include/config.h](include/config.h)
* 3D printable case (OpenSCAD): [case/oil_meter_case.scad](case/oil_meter_case.scad)
* WiFi / MQTT credentials: `include/secrets.h` (template: `include/secrets.example.h`)

## Setup

1. Install [PlatformIO](https://platformio.org/) (VS Code extension or `pip install platformio`).
2. Edit `include/secrets.h` with your WiFi and MQTT broker details.
3. Edit the tank section in `include/config.h`:
   * `TANK_SHAPE`: `Rectangular`, `VerticalCylinder`, `HorizontalCylinder` or `Table`
   * the matching inner dimensions in cm, `TANK_COUNT` for battery tanks
   * `SENSOR_OFFSET_CM`: distance from the probe face to the inner tank top
4. Build:

   ```sh
   pio run -e firebeetle2_c5_debug   # serial log, 60 s interval – for setup/calibration
   pio run -e firebeetle2_c5         # release: silent, 6 h interval
   pio run -e firebeetle2_c5_zigbee  # release, reports via Zigbee instead of WiFi/MQTT
   pio test -e native                # unit tests of the volume math on your PC
   ```

5. Upload with `pio run -e <env> -t upload`. Once the firmware runs, the board spends
   most of its time in deep sleep without USB; to flash again hold **BOOT**, tap
   **RST**, release BOOT and upload.

## Calibrating with an Android phone

For calibrating at the tank without a laptop there is a firmware variant that streams
every ping over USB and a small Android app (`android/`) that shows it live.

1. Flash the streaming firmware: `pio run -e firebeetle2_c5_calibration -t upload`
   (hold **BOOT**, tap **RST** if the board is asleep). It never sleeps and has no WiFi.
2. Build and install the app: `cd android && ./gradlew installDebug` (or open `android/`
   in Android Studio). Needs JDK 17 and the Android SDK.
3. Plug the FireBeetle into the phone with a USB-C OTG cable (the phone powers the board and
   sensor) and allow USB access. The app also starts when the board is plugged in.

The app shows the median of the last 9 pings (like the real firmware), a strip chart,
statistics and the raw echo time. Set the air temperature, enter the tape-measured
distance and it suggests the `SENSOR_OFFSET_CM` or `AIR_TEMPERATURE_C` correction.
The firmware sends only raw echo times (`P,<seq>,<echo_us>`), so all of this is tuned
live in the app without reflashing. Protocol: [calibration/main.cpp](calibration/main.cpp).

## MQTT topics

All values are published retained under `oilmeter/<DEVICE_ID>/`:

| Topic | Unit | Description |
|-------|------|-------------|
| `volume` | L | Oil volume (all tanks) |
| `fill` | % | Fill level |
| `free` | L | Free capacity – how much can be ordered |
| `height` | cm | Oil height above the tank bottom |
| `distance` | cm | Raw sensor distance (median) |
| `battery` | % | Estimated battery charge |
| `battery_voltage` | V | Battery voltage |
| `rssi` | dBm | WiFi signal strength |
| `status` | – | `ok` or `sensor_error` |

With `HA_DISCOVERY_ENABLED` all of them appear automatically as one device in Home
Assistant. Values expire there if the device stops reporting. Other systems
(ioBroker, openHAB, Node-RED, …) can subscribe to the topics directly.

## Zigbee instead of WiFi

The ESP32-C5 also has an IEEE 802.15.4 radio, so the firmware can report to an existing
Zigbee network (Zigbee2MQTT, ZHA, deCONZ, …) instead of WiFi/MQTT. It is chosen at
build time, as it needs a different partition table and the Zigbee stack:

```sh
pio run -e firebeetle2_c5_zigbee_debug -t upload   # serial log, 5 s interval
pio run -e firebeetle2_c5_zigbee -t upload         # release: silent, 6 h interval
```

`secrets.h` is not needed. Zigbee settings (model name, timeouts) are in the Zigbee
section of `include/config.h`. The two firmwares use different partition tables, so
erase the flash once when switching (`pio run -e <env> -t erase`, then upload).

**Pairing:** enable *permit join* on the coordinator, then press **RST**. The LED is
on while the board searches for a network (up to 3 min). After joining it stays awake
for 30 s so the coordinator can interview it, then it reports every 6 h like the WiFi
version. The network is remembered across deep sleep and battery changes.

**Joining a different network:** press **RST**, then press **BOOT** while the LED is
lit (first 3 s). The LED blinks, the old network is forgotten and pairing starts.

The device is a battery powered end device with one endpoint per value, as Analog
Input clusters (with description and unit), plus the standard battery cluster:

| Endpoint | Value | Unit |
|----------|-------|------|
| 10 | Oil volume (+ battery %) | L |
| 11 | Fill level | % |
| 12 | Free capacity | L |
| 13 | Oil height | cm |
| 14 | Sensor distance | cm |
| 15 | Battery voltage | V |
| 16 | Sensor error (Binary Input) | – |

* **Zigbee2MQTT:** copy [zigbee2mqtt/oil_meter.mjs](zigbee2mqtt/oil_meter.mjs) to
  `<z2m data>/external_converters/` and restart Zigbee2MQTT. The device then shows
  up with named values (`volume`, `fill`, `free`, …) and in Home Assistant via the
  Zigbee2MQTT discovery.
* **ZHA:** the values appear as sensors named after their description (*Oil volume*,
  *Fill level*, …) with their units (L, %, cm, V), taken from the EngineeringUnits
  attribute. The firmware deliberately leaves out the ApplicationType attribute, since
  ZHA would take the unit from that instead and there is no type for volume or length.

Reports are sent directly to the coordinator (no binding needed) and count as
delivered once acknowledged. Failed reports retry with the same backoff as WiFi.

## How it saves power

* Deep sleep between measurements (default every 6 h, 24 h when the battery is low,
  no WiFi at all when it is critical).
* Sensor supply cut completely: it runs from the board's switchable 3V3_C output.
* Measurement happens before the radio is switched on; CPU runs at 80 MHz.
* WiFi BSSID and channel are cached in RTC memory to skip scanning; optional static IP.
* Zigbee: the network channel is stored in flash and the parent is asked to keep the
  device for ~11 days, so a wakeup is a short rejoin on one channel.
* Failed reports retry with exponential backoff (15 min, 30 min, … up to the
  regular interval) instead of retrying in a tight loop.
