#pragma once
#include <Arduino.h>
#include "Ads1115Sampler.h"
#define TEMP_AVERAGE_SAMPLES 15
class TemperatureSensor {
public:
    TemperatureSensor(const char* name, Ads1115Sampler& sampler, uint8_t channel);
    float readCelsius();
    const char* name() const { return _name; }
    int pin() const { return _channel; }
private:
    const char* _name;
    Ads1115Sampler& _sampler;
    uint8_t _channel;
    uint32_t _lastSequence = 0;
    float _samples[TEMP_AVERAGE_SAMPLES] = {};
    size_t _index = 0, _count = 0;
    float _sum = 0.0f, _average = NAN;
};
