#include "tank_geometry.h"

#include <math.h>

namespace tank {
namespace {

constexpr float kPi = 3.14159265358979f;
constexpr float kCm3PerLiter = 1000.0f;

float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// Volume of a single tank in liters.
float singleTankLiters(const Config& cfg, float h) {
    switch (cfg.shape) {
        case Shape::Rectangular:
            return cfg.lengthCm * cfg.widthCm * h / kCm3PerLiter;

        case Shape::VerticalCylinder: {
            const float r = cfg.diameterCm / 2.0f;
            return kPi * r * r * h / kCm3PerLiter;
        }

        case Shape::HorizontalCylinder: {
            // Area of a circular segment of height h, times the cylinder length.
            const float r = cfg.diameterCm / 2.0f;
            const float area = r * r * acosf((r - h) / r) - (r - h) * sqrtf(2.0f * r * h - h * h);
            return area * cfg.lengthCm / kCm3PerLiter;
        }

        case Shape::Table: {
            if (cfg.table == nullptr || cfg.tableSize == 0) return 0.0f;
            // Below the first point: interpolate from (0 cm, 0 L).
            float prevH = 0.0f;
            float prevL = 0.0f;
            for (size_t i = 0; i < cfg.tableSize; ++i) {
                const TablePoint& p = cfg.table[i];
                if (h <= p.heightCm) {
                    if (p.heightCm <= prevH) return p.liters;
                    return prevL + (p.liters - prevL) * (h - prevH) / (p.heightCm - prevH);
                }
                prevH = p.heightCm;
                prevL = p.liters;
            }
            return cfg.table[cfg.tableSize - 1].liters;
        }
    }
    return 0.0f;
}

}  // namespace

float innerHeightCm(const Config& cfg) {
    switch (cfg.shape) {
        case Shape::HorizontalCylinder:
            return cfg.diameterCm;
        case Shape::Table:
            return (cfg.table && cfg.tableSize) ? cfg.table[cfg.tableSize - 1].heightCm : 0.0f;
        default:
            return cfg.heightCm;
    }
}

float litersAtHeight(const Config& cfg, float fillHeightCm) {
    const float h = clampf(fillHeightCm, 0.0f, innerHeightCm(cfg));
    const unsigned count = cfg.count ? cfg.count : 1;
    return singleTankLiters(cfg, h) * static_cast<float>(count);
}

float capacityLiters(const Config& cfg) {
    return litersAtHeight(cfg, innerHeightCm(cfg));
}

Level levelFromDistance(const Config& cfg, float distanceCm) {
    const float maxH = innerHeightCm(cfg);
    Level lvl;
    lvl.fillHeightCm = clampf(maxH + cfg.sensorOffsetCm - distanceCm, 0.0f, maxH);
    lvl.liters = litersAtHeight(cfg, lvl.fillHeightCm);
    lvl.capacityLiters = capacityLiters(cfg);
    lvl.percent = lvl.capacityLiters > 0.0f ? lvl.liters / lvl.capacityLiters * 100.0f : 0.0f;
    return lvl;
}

}  // namespace tank
