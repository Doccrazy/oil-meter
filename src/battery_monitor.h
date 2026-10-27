#pragma once

namespace battery {

// Battery voltage via the external divider on PIN_BATTERY_ADC, in volts.
float readVolts();

}  // namespace battery
