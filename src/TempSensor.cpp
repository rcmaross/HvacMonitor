#include "TempSensor.h"
#include <math.h>
TemperatureSensor::TemperatureSensor(const char* name, Ads1115Sampler& sampler, uint8_t channel)
    : _name(name), _sampler(sampler), _channel(channel) {}
float TemperatureSensor::readCelsius() {
    int16_t raw;
    uint32_t sequence;
    if (!_sampler.latest(_channel, raw, sequence) || sequence == _lastSequence) return _average;
    _lastSequence = sequence;
    const float volts = raw * 0.000125f;
    if (volts <= 0.0f || volts >= 3.3f) return _average;
    const float resistance = 10000.0f * volts / (3.3f - volts);
    const float invKelvin = logf(resistance / 10000.0f) / 3950.0f + 1.0f / 298.15f;
    const float celsius = 1.0f / invKelvin - 273.15f;
    if (!isfinite(celsius)) return _average;
    if (_count == TEMP_AVERAGE_SAMPLES) _sum -= _samples[_index];
    else ++_count;
    _samples[_index] = celsius;
    _sum += celsius;
    _index = (_index + 1) % TEMP_AVERAGE_SAMPLES;
    return _average = _sum / _count;
}
