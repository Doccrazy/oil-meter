#include "battery_monitor.h"

#include <Arduino.h>

#include "config.h"

namespace battery {

float readVolts() {
    constexpr int kSamples = 16;
    uint32_t sumMv = 0;
    for (int i = 0; i < kSamples; ++i) {
        sumMv += analogReadMilliVolts(cfg::PIN_BATTERY_ADC);  // eFuse calibrated
    }
    const float pinVolts = static_cast<float>(sumMv) / kSamples / 1000.0f;
    const float ratio = (cfg::BATTERY_DIVIDER_R_TOP + cfg::BATTERY_DIVIDER_R_BOTTOM) / cfg::BATTERY_DIVIDER_R_BOTTOM;
    return pinVolts * ratio * cfg::BATTERY_CALIBRATION;
}

}  // namespace battery
