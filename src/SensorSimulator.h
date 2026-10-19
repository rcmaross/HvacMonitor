#pragma once
#include <Arduino.h>

// Bench-only physical loopback: DAC25->ADC35, DAC26->ADC36,
// PWM19 through external RC filter -> ADC34.
class SensorSimulator {
public:
    void begin();
    void update();
    bool active() const { return _active; }
private:
    void process(char* line);
    void setZero();
    char _input[96]={};
    size_t _used=0;
    bool _active=false;
    bool _initialized=false;
    uint32_t _lastUpdate=0;
};
