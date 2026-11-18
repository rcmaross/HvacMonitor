#pragma once
#include <Arduino.h>
#include "Ads1115Sampler.h"
#define CURRENT_AVERAGE_SAMPLES 15
class CurrentSensor {
public:
    void begin(Ads1115Sampler& sampler, uint8_t channel);
    float readAmps();
private:
    Ads1115Sampler* _sampler = nullptr;
    uint8_t _channel = 0;
    uint32_t _lastSequence = 0;
    float _samples[CURRENT_AVERAGE_SAMPLES] = {};
    size_t _index = 0, _count = 0;
    float _sum = 0.0f, _average = NAN;
};
