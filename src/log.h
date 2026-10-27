#pragma once

#include <Arduino.h>

#include "config.h"

// Serial logging only in the debug environment; the release build stays silent to
// keep every wakeup as short as possible.
#if OILMETER_DEBUG
#define LOGF(...) Serial.printf(__VA_ARGS__)
#else
#define LOGF(...) \
    do {          \
    } while (0)
#endif
