#pragma once
#include <Arduino.h>
#include "Status.h"

// Bench-only physical loopback: DAC25->ADC35, DAC26->ADC36,
// PWM19 through external RC filter -> ADC34.
class SensorSimulator {
public:
    void begin(Status& status);
    void reportTemperatures();
    void update();
    bool active() const { return _active; }
private:
    void process(char* line);
    void setZero();
    Status* _status=nullptr;
    int _previous[3]={INT32_MIN,INT32_MIN,INT32_MIN};
    char _input[96]={};
    size_t _used=0;
    bool _active=false;
    bool _initialized=false;
    uint32_t _lastUpdate=0;
};
