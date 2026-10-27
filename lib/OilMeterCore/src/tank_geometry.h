#pragma once

#include <stddef.h>

// Pure tank math (no Arduino dependencies, unit tested on the host).
namespace tank {

enum class Shape {
    Rectangular,         // box: lengthCm x widthCm x heightCm
    VerticalCylinder,    // standing cylinder: diameterCm x heightCm
    HorizontalCylinder,  // lying cylinder: diameterCm x lengthCm (height = diameter)
    Table,               // calibration table ("Peiltabelle"): height -> liters
};

struct TablePoint {
    float heightCm;
    float liters;
};

struct Config {
    Shape shape;
    float heightCm;     // inner height (ignored for HorizontalCylinder and Table)
    float lengthCm;     // Rectangular, HorizontalCylinder
    float widthCm;      // Rectangular
    float diameterCm;   // VerticalCylinder, HorizontalCylinder
    unsigned count;     // number of identical tanks connected together (battery tanks)
    const TablePoint* table;  // Table only, sorted by ascending heightCm, per single tank
    size_t tableSize;
    // Distance between the sensor face and the highest possible oil level (inner tank
    // top). Use this when the sensor sits in a riser/fitting above the tank top.
    float sensorOffsetCm;
};

struct Level {
    float fillHeightCm;    // oil height above tank bottom
    float liters;          // total volume over all tanks
    float capacityLiters;  // total capacity over all tanks
    float percent;         // liters / capacity * 100
};

// Maximum fill height of a single tank in cm.
float innerHeightCm(const Config& cfg);

// Total volume (all tanks) at the given fill height.
float litersAtHeight(const Config& cfg, float fillHeightCm);

// Total capacity (all tanks).
float capacityLiters(const Config& cfg);

// Convert a measured sensor-to-surface distance into a fill level.
Level levelFromDistance(const Config& cfg, float distanceCm);

}  // namespace tank
