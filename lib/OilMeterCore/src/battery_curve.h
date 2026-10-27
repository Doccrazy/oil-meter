#pragma once

namespace battery {

// Approximate state of charge (0..100 %) of a single Li-ion / LiPo cell from its
// resting voltage. Good enough for a "replace/charge soon" indication.
float lipoPercent(float volts);

}  // namespace battery
