// Zigbee2MQTT external converter for the oil level monitor (firebeetle2_c5_zigbee firmware).
//
// Copy this file to <zigbee2mqtt data>/external_converters/ and restart Zigbee2MQTT.
// Endpoint layout: src/transport_zigbee.cpp. If you change ZIGBEE_MODEL in
// include/config.h, change zigbeeModel below too.

import * as exposes from 'zigbee-herdsman-converters/lib/exposes';

const e = exposes.presets;
const ea = exposes.access;

// endpoint -> [property, decimals]
const ANALOG_ENDPOINTS = {
    10: ['volume', 0],
    11: ['fill', 1],
    12: ['free', 0],
    13: ['height', 1],
    14: ['distance', 1],
    15: ['battery_voltage', 2],
};

const fzAnalog = {
    cluster: 'genAnalogInput',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg) => {
        const entry = ANALOG_ENDPOINTS[msg.endpoint.ID];
        const value = msg.data.presentValue;
        if (!entry || typeof value !== 'number') return;
        const [property, decimals] = entry;
        const factor = 10 ** decimals;
        return {[property]: Math.round(value * factor) / factor};
    },
};

const fzSensorError = {
    cluster: 'genBinaryInput',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg) => {
        const value = msg.data.presentValue;
        if (value === undefined) return;
        return {sensor_error: value === true || value === 1};
    },
};

const fzBattery = {
    cluster: 'genPowerCfg',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg) => {
        const value = msg.data.batteryPercentageRemaining;
        if (typeof value !== 'number' || value === 255) return;
        return {battery: Math.round(value / 2)};
    },
};

export default {
    zigbeeModel: ['OilMeter'],
    model: 'OilMeter',
    vendor: 'DIY',
    description: 'Heating oil level monitor (FireBeetle 2 ESP32-C5 + JSN-SR04T)',
    fromZigbee: [fzAnalog, fzSensorError, fzBattery],
    toZigbee: [],
    exposes: [
        e.numeric('volume', ea.STATE).withUnit('L').withDescription('Oil volume (all tanks)'),
        e.numeric('fill', ea.STATE).withUnit('%').withDescription('Fill level'),
        e.numeric('free', ea.STATE).withUnit('L').withDescription('Free capacity – how much can be ordered'),
        e.numeric('height', ea.STATE).withUnit('cm').withDescription('Oil height above the tank bottom'),
        e.numeric('distance', ea.STATE).withUnit('cm').withDescription('Raw sensor distance (median)'),
        e.battery(),
        e.numeric('battery_voltage', ea.STATE).withUnit('V').withDescription('Battery voltage'),
        e.binary('sensor_error', ea.STATE, true, false).withDescription('Ultrasonic sensor gave no valid reading'),
    ],
};
