#include "sensor.h"

#include <Arduino.h>
#include <math.h>

#include <algorithm>

#include "config.h"
#include "log.h"

namespace sensor {
namespace {

float speedOfSoundCmPerUs() {
    return (331.3f + 0.606f * cfg::AIR_TEMPERATURE_C) / 10000.0f;
}

// Round trip time at maximum range plus 20 % margin.
unsigned long echoTimeoutUs() {
    return static_cast<unsigned long>(2.0f * cfg::SENSOR_MAX_DISTANCE_CM / speedOfSoundCmPerUs() * 1.2f);
}

}  // namespace

unsigned long pingEchoUs() {
    digitalWrite(cfg::PIN_SENSOR_TRIG, LOW);
    delayMicroseconds(5);
    // The JSN-SR04T needs a longer trigger pulse than the HC-SR04 (>10 us).
    digitalWrite(cfg::PIN_SENSOR_TRIG, HIGH);
    delayMicroseconds(20);
    digitalWrite(cfg::PIN_SENSOR_TRIG, LOW);

    return pulseIn(cfg::PIN_SENSOR_ECHO, HIGH, echoTimeoutUs());
}

namespace {

float pingCm() {
    const unsigned long us = pingEchoUs();
    if (us == 0) return NAN;
    return static_cast<float>(us) * speedOfSoundCmPerUs() / 2.0f;
}

}  // namespace

void powerOff() {
    // Pull TRIG low first so the unpowered sensor is not fed through its input.
    pinMode(cfg::PIN_SENSOR_TRIG, OUTPUT);
    digitalWrite(cfg::PIN_SENSOR_TRIG, LOW);
    pinMode(cfg::PIN_SENSOR_TRIG, INPUT);
    pinMode(cfg::PIN_SENSOR_ECHO, INPUT);
    // Gate floats -> the 100k pull-up keeps the P-MOSFET off, also during deep sleep.
    pinMode(cfg::PIN_SENSOR_POWER, INPUT);
}

void powerOn() {
    pinMode(cfg::PIN_SENSOR_ECHO, INPUT);
    pinMode(cfg::PIN_SENSOR_TRIG, OUTPUT);
    digitalWrite(cfg::PIN_SENSOR_TRIG, LOW);
    pinMode(cfg::PIN_SENSOR_POWER, OUTPUT);
    digitalWrite(cfg::PIN_SENSOR_POWER, LOW);  // P-MOSFET on
    delay(cfg::SENSOR_WARMUP_MS);
}

Reading measure() {
    powerOn();

    float samples[cfg::SENSOR_SAMPLES];
    int n = 0;
    for (int i = 0; i < cfg::SENSOR_SAMPLES; ++i) {
        const float d = pingCm();
        const bool valid = !isnan(d) && d >= cfg::SENSOR_MIN_DISTANCE_CM && d <= cfg::SENSOR_MAX_DISTANCE_CM;
        LOGF("  ping %d: %.1f cm%s\n", i, d, valid ? "" : " (rejected)");
        if (valid) samples[n++] = d;
        delay(cfg::SENSOR_PING_INTERVAL_MS);
    }
    powerOff();

    Reading r{false, NAN, n};
    if (n < cfg::SENSOR_MIN_VALID_SAMPLES) return r;

    std::sort(samples, samples + n);
    r.distanceCm = (n % 2) ? samples[n / 2] : (samples[n / 2 - 1] + samples[n / 2]) / 2.0f;
    r.ok = true;
    return r;
}

}  // namespace sensor
