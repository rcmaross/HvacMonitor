#include "CurrentSensor.h"
#include <math.h>
void CurrentSensor::begin(Ads1115Sampler& sampler, uint8_t channel) {
    _sampler = &sampler;
    _channel = channel;
}
float CurrentSensor::readAmps() {
    int16_t raw;
    uint32_t sequence;
    if (!_sampler || !_sampler->latest(_channel, raw, sequence) || sequence == _lastSequence) return _average;
    _lastSequence = sequence;
    const float amps = fmaxf(0.0f, raw * 0.000125f) * (100.0f / 3.3f);
    if (!isfinite(amps)) return _average;
    if (_count == CURRENT_AVERAGE_SAMPLES) _sum -= _samples[_index];
    else ++_count;
    _samples[_index] = amps;
    _sum += amps;
    _index = (_index + 1) % CURRENT_AVERAGE_SAMPLES;
    return _average = _sum / _count;
}
