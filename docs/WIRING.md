# Wiring

## Parts

| Ref | Part | Notes |
|-----|------|-------|
| U1 | Seeed Studio XIAO ESP32C6 | |
| U2 | JSN-SR04T (V3.x) waterproof ultrasonic sensor | Leave it in **mode 1** (trigger/echo, mode resistor R27 not fitted) |
| Q1 | AO3401A P-channel MOSFET (SOT-23) | High-side switch for the sensor supply. A SOT-23 breakout board makes soldering much easier |
| R1 | 1 kΩ | Gate series resistor |
| R2 | 100 kΩ | Gate pull-up, keeps the sensor **off** during sleep and boot |
| R3, R4 | 1 MΩ each (1 % if possible) | Battery voltage divider |
| C1 | 100 nF ceramic | Sensor supply decoupling, close to the sensor board |
| C2 | 10 µF ceramic/electrolytic (optional) | Extra sensor buffering, see *Troubleshooting* |
| C3 | 100 nF ceramic | ADC filter / sample cap for the high-impedance divider |
| BT1 | 1-cell Li-ion / LiPo (3.7 V nominal), with protection circuit | Charged by the XIAO through USB-C |

## Pin assignment

| XIAO pin | GPIO | Connected to | Configured in `include/config.h` |
|----------|------|--------------|----------------------------------|
| D0 / A0 | GPIO0 | Battery divider midpoint (R3/R4) + C3 | `PIN_BATTERY_ADC` |
| D1 | GPIO1 | Q1 gate via R1 (LOW = sensor on) | `PIN_SENSOR_POWER` |
| D2 | GPIO2 | JSN-SR04T **Trig** (sometimes labelled RX) | `PIN_SENSOR_TRIG` |
| D3 | GPIO21 | JSN-SR04T **Echo** (sometimes labelled TX) | `PIN_SENSOR_ECHO` |
| 3V3 | – | Q1 source, R2 | |
| GND | – | Common ground | |
| BAT+ / BAT− pads (back side) | – | Battery | |

```
                 XIAO ESP32C6 (top view)
                 ┌──────────[USB-C]──────────┐
  battery sense ─┤ D0 (GPIO0)          5V    ├─ (not used)
   MOSFET gate  ─┤ D1 (GPIO1)          GND   ├─ GND
   sensor TRIG  ─┤ D2 (GPIO2)          3V3   ├─ Q1 source / R2
   sensor ECHO  ─┤ D3 (GPIO21)         D10   │
                 │ D4                  D9    │
                 │ D5                  D8    │
                 │ D6                  D7    │
                 └───────────────────────────┘
                    back side: BAT+  BAT−  pads
```

## Schematic

### Sensor power switch and signals

```
 XIAO 3V3 ───────────┬────────────────────┐
                     │                    │ S (pin 2)
                    R2 100k            ┌──┴──┐
                     │           G     │ Q1  │  AO3401A, P-channel
 XIAO D1 ──[R1 1k]───┴─────────────────┤     │
                              (pin 1)  └──┬──┘
                                          │ D (pin 3)
                                          │
                                          ├──────┬──────┬──── JSN-SR04T  5V / VCC
                                          │      │      │
                                         C1    C2 +     │
                                        100n  10µ(opt)  │
                                          │      │      │
 XIAO GND ────────────────────────────────┴──────┴──────┴──── JSN-SR04T  GND

 XIAO D2 ─────────────────────────────────────────────────── JSN-SR04T  Trig
 XIAO D3 ─────────────────────────────────────────────────── JSN-SR04T  Echo
```

AO3401A pinout (SOT-23, marking facing you, single pin on top):

```
          3 (Drain)
        ┌───┴───┐
        │ A1**  │
        └─┬───┬─┘
  (Gate) 1     2 (Source)
```

Always double check against the datasheet of your exact part / breakout board.

### Battery and voltage measurement

```
 Battery + ──┬──────────────────────── XIAO BAT+ pad
             │
            R3 1M
             │
             ├──────────┬───────────── XIAO D0 (GPIO0)
             │          │
            R4 1M      C3 100n
             │          │
 Battery − ──┴──────────┴───────────── XIAO BAT− pad  (= GND)
```

The divider halves the battery voltage (max. 4.2 V → 2.1 V) to stay inside the
ADC range. With 2 × 1 MΩ it permanently draws only ~2 µA. C3 is required: the ADC
cannot sample a 500 kΩ source directly, the capacitor provides the charge.

## Why it is wired this way (low power design)

* **Sensor fully switched off.** The JSN-SR04T draws several mA even when idle,
  which would drain the battery in weeks. Q1 cuts its supply completely between
  measurements.
* **Sensor powered from 3V3, not from the battery.** With the MOSFET source on 3V3,
  a HIGH (3.3 V) on the gate turns it fully off and the sensor's Echo output is
  3.3 V logic, so no level shifter is needed. If the gate were tied to VBAT
  (up to 4.2 V) a 3.3 V GPIO could not switch the P-MOSFET off reliably.
  The JSN-SR04T V3.x is specified for 3.0–5.5 V.
* **R2 pull-up.** In deep sleep and during boot the GPIO is high-impedance; R2
  keeps the gate at 3V3, so the sensor stays off without any firmware involvement.
  The firmware also switches TRIG/ECHO to inputs before sleeping so the unpowered
  sensor is not back-fed through its signal pins.
* **No LEDs, no USB serial in the release build.** The release firmware stays
  silent, measures with the radio off and only enables WiFi for the MQTT publish.

### Expected power budget (estimate)

| Phase | Current | Duration |
|-------|---------|----------|
| Deep sleep (XIAO + divider) | ~15–20 µA | rest of the time |
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

1. Build/flash the debug variant (`pio run -e xiao_esp32c6_debug -t upload`) and open
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
| Board resets when the sensor switches on (log shows "reset" instead of "timer" wakeups) | Inrush current into the sensor capacitors dips the 3V3 rail. Remove C2, or increase R1 to 10 kΩ to slow the MOSFET turn-on |
| Always `sensor_error` | Check mode resistor (must be mode 1), Trig/Echo swapped, tank full (blind zone), increase `SENSOR_WARMUP_MS` |
| Readings jump | Side reflections – move/align the probe; increase `SENSOR_SAMPLES` |
| Range too short | Some JSN-SR04T boards perform worse at 3.3 V. Alternative: power the sensor from BAT+ via Q1, but then add an NPN/N-MOSFET stage to drive the gate and a 10k/20k divider on Echo |
| Battery shows nothing | Below 2.5 V the firmware assumes "no battery" (e.g. USB only) and does not publish battery values |
| Can't flash after the first upload | The chip is in deep sleep and USB is gone: hold **BOOT**, press **RESET**, release BOOT, then upload |
