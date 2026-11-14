#include "CurrentSensor.h"
#include <math.h>

// Conditioned DC sensor output: 0-3.3 V corresponds to 0-100 A.
// This is a nominal transfer function; validate against the actual sensor.
float CurrentSensor::readAmps() {
    if (_pin < 0) return NAN;
    constexpr float FULL_SCALE_MV = 3300.0f;
    constexpr float FULL_SCALE_AMPS = 100.0f;
    const uint32_t millivolts = analogReadMilliVolts(_pin);
    return (static_cast<float>(millivolts) / FULL_SCALE_MV) * FULL_SCALE_AMPS;
}
