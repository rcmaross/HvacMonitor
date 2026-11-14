#include "CurrentSensor.h"
#include <math.h>
// AC RMS acquisition with DC bias removed. Calibration intentionally withheld
// until the CT interface, burden, and bias circuit are confirmed.
float CurrentSensor::readAmps() {
    if (_pin < 0) return NAN;
    // Do not report a fictitious amperage from uncalibrated ADC counts.
    return NAN;
}
