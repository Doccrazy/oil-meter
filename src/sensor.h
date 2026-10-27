#pragma once

// JSN-SR04T ultrasonic sensor, powered through the AO3401A high-side switch.
namespace sensor {

struct Reading {
    bool ok;
    float distanceCm;  // median of the valid pings, NAN if !ok
    int validSamples;
};

// Switch the sensor on, take the configured number of pings, switch it off again.
Reading measure();

// Sensor unpowered and all its pins high impedance (safe for deep sleep).
void powerOff();

}  // namespace sensor
