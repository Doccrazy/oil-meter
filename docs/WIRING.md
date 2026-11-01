# Wiring

## Parts

| Ref | Part | Notes |
|-----|------|-------|
| U1 | DFRobot FireBeetle 2 ESP32-C5 (DFR1222) | Has a switchable 3.3 V output (3V3_C) and on-board battery voltage sensing |
| U2 | JSN-SR04T (V3.x) waterproof ultrasonic sensor | Leave it in **mode 1** (trigger/echo, mode resistor R27 not fitted) |
| C1 | 100 nF ceramic | Sensor supply decoupling, close to the sensor board |
| C2 | 10 µF ceramic/electrolytic (optional) | Extra sensor buffering, see *Troubleshooting* |
| BT1 | 1-cell Li-ion / LiPo (3.7 V nominal), with protection circuit | 2-pin JST PH (2.0 mm) plug for the BAT connector, charged through USB-C (or a solar panel on VIN) |

No MOSFET, gate resistors or battery divider are needed: the board already has them.

## Pin assignment

| FireBeetle pin | GPIO | Connected to | Configured in `include/config.h` |
|----------------|------|--------------|----------------------------------|
| 3V3_C | – (switched by IO0) | JSN-SR04T **5V / VCC**, C1, C2 | `PIN_SENSOR_POWER` = 0 (HIGH = on) |
| GND | – | JSN-SR04T **GND**, C1, C2 | |
| 8/D2 | GPIO8 | JSN-SR04T **Trig** (sometimes labelled RX) | `PIN_SENSOR_TRIG` |
| 26/D3 | GPIO26 | JSN-SR04T **Echo** (sometimes labelled TX) | `PIN_SENSOR_ECHO` |
| BAT | – | Battery (2-pin PH2.0 = JST PH, 2.0 mm pitch) | |
| – | GPIO1 | On-board battery divider, nothing to wire | `PIN_BATTERY_ADC` |

Avoid GPIO7, 25, 27 and 28 for the sensor: they are strapping pins / the BOOT button.
GPIO15 is the on-board LED.

## Schematic

```
 FireBeetle 3V3_C ──────────┬──────┬──────────── JSN-SR04T  5V / VCC
 (on while IO0 HIGH)        │      │
                           C1    C2 +
                          100n  10µ(opt)
                            │      │
 FireBeetle GND ────────────┴──────┴──────────── JSN-SR04T  GND

 FireBeetle 8/D2  ────────────────────────────── JSN-SR04T  Trig
 FireBeetle 26/D3 ────────────────────────────── JSN-SR04T  Echo

 Battery ──────────────────────────────────────── FireBeetle BAT connector
```

Check the polarity of the battery's JST plug against the BAT marking on the board.
Many cells are sold with the opposite pinout.

## Why it is wired this way (low power design)

* **Sensor fully switched off.** The JSN-SR04T draws several mA even when idle,
  which would drain the battery in weeks. The board's 3V3_C output is enabled only
  while IO0 is driven HIGH. It is off by default, so the sensor stays unpowered during
  deep sleep and boot without any firmware involvement.
* **Sensor powered from 3.3 V, not from the battery.** Its Echo output is then 3.3 V
  logic, so no level shifter is needed. The JSN-SR04T V3.x is specified for 3.0–5.5 V.
  The firmware also switches TRIG/ECHO to inputs before sleeping so the unpowered
  sensor is not back-fed through its signal pins.
* **Battery sensing on board.** IO1 reads the battery through the board's divider
  (assumed 1:1, see `BATTERY_DIVIDER_R_*`). Calibrate it once with a multimeter.
* **No LEDs, no USB serial in the release build.** The release firmware stays
  silent, measures with the radio off and only enables WiFi for the MQTT publish.

### Expected power budget (estimate)

| Phase | Current | Duration |
|-------|---------|----------|
| Deep sleep (board, 21 µA per DFRobot) | ~20–25 µA | rest of the time |
| Sensor measurement (500 ms warm-up + 9 pings) | ~10–20 mA | ~1.2 s |
| WiFi connect + MQTT (cached BSSID/channel) | ~80–120 mA avg | ~1–2 s (more with DHCP) |

At the default interval of 6 h this is roughly 0.5–1 mAh per day, i.e. a 2000 mAh
cell lasts well over a year; self-discharge of the cell becomes the limiting factor.
A static IP (`USE_STATIC_IP`) and a broker IP instead of a hostname shorten every
wakeup further.

## Mounting the sensor

```
          JSN-SR04T probe
              ▼
         ┌────┴────┐  ← riser / fitting
         │         │     SENSOR_OFFSET_CM = distance probe face → inner tank top
   ┌─────┴─────────┴─────┐  ← inner tank top (max. oil level)
   │          ▲          │
   │          │ measured │
   │          │ distance │
   │~~~~~~~~~~▼~~~~~~~~~~│  ← oil surface
   │                     │  fill height = TANK_HEIGHT_CM + SENSOR_OFFSET_CM − distance
   │                     │
   └─────────────────────┘  ← tank bottom
```

* **Blind zone:** the JSN-SR04T cannot measure below ~20–25 cm. Mount it so that
  the distance to the *highest* possible oil level is larger than that (e.g. on a
  riser pipe) and set `SENSOR_OFFSET_CM` accordingly. A full tank otherwise shows
  up as `sensor_error`.
* Point the probe straight down, away from the fill pipe, the suction line and the
  tank walls; the sound cone is fairly wide and side reflections cause wrong readings.
* Use a free, existing opening of the tank (e.g. the spare fitting of a battery tank)
  and seal it vapour-tight. Only the probe goes into the tank, keep the electronics
  outside. Check your local rules for heating oil installations before modifying
  the tank.
* For battery tank systems mount the sensor in one tank and set `TANK_COUNT`.

## Calibration

Quickest: the Android app with the USB streaming firmware, see the README. Or with
the serial log:

1. Build/flash the debug variant (`pio run -e firebeetle2_c5_debug -t upload`) and open
   the serial monitor (`pio device monitor`). It measures every 60 s and logs every ping.
2. Compare the reported *height* with a dipstick reading or the tank's level gauge.
3. Adjust `SENSOR_OFFSET_CM` (constant error) or `AIR_TEMPERATURE_C` (error that
   grows with distance). If the tank has a manufacturer calibration table,
   switch to `tank::Shape::Table` and enter it in `TANK_TABLE`.
4. Measure the battery voltage with a multimeter and set `BATTERY_CALIBRATION =
   multimeter / reported`.
5. Flash the release variant.

## Troubleshooting

| Symptom | Cause / fix |
|---------|-------------|
| Board resets when the sensor switches on (log shows "reset" instead of "timer" wakeups) | Inrush current into the sensor capacitors dips the 3V3 rail. Remove C2 |
| Always `sensor_error` | Check mode resistor (must be mode 1), Trig/Echo swapped, tank full (blind zone), increase `SENSOR_WARMUP_MS` |
| Readings jump | Side reflections – move/align the probe; increase `SENSOR_SAMPLES` |
| Range too short | Some JSN-SR04T boards perform worse at 3.3 V. Alternative: power the sensor from BAT through a P-MOSFET high-side switch driven by an NPN/N-MOSFET stage, and add a 10k/20k divider on Echo |
| Battery shows nothing | Below 2.5 V the firmware assumes "no battery" (e.g. USB only) and does not publish battery values |
| Battery voltage about half or double the multimeter value | The on-board divider is not 1:1 on your board revision: adjust `BATTERY_DIVIDER_R_TOP/BOTTOM` |
| Can't flash after the first upload | The chip is in deep sleep and USB is gone: hold **BOOT**, press **RST**, release BOOT, then upload |
