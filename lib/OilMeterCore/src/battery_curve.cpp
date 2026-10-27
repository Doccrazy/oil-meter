#include "battery_curve.h"

namespace battery {
namespace {

struct Point {
    float volts;
    float percent;
};

// Typical discharge curve of a LiPo cell at low load, descending voltage.
constexpr Point kCurve[] = {
    {4.20f, 100.0f}, {4.10f, 90.0f}, {4.00f, 80.0f}, {3.92f, 70.0f}, {3.87f, 60.0f},
    {3.83f, 50.0f},  {3.79f, 40.0f}, {3.75f, 30.0f}, {3.71f, 20.0f}, {3.65f, 10.0f},
    {3.50f, 5.0f},   {3.30f, 0.0f},
};
constexpr int kCurveSize = sizeof(kCurve) / sizeof(kCurve[0]);

}  // namespace

float lipoPercent(float volts) {
    if (volts >= kCurve[0].volts) return 100.0f;
    for (int i = 1; i < kCurveSize; ++i) {
        if (volts >= kCurve[i].volts) {
            const Point& hi = kCurve[i - 1];
            const Point& lo = kCurve[i];
            return lo.percent + (hi.percent - lo.percent) * (volts - lo.volts) / (hi.volts - lo.volts);
        }
    }
    return 0.0f;
}

}  // namespace battery
