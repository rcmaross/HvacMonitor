#pragma once
#include <Arduino.h>
class CurrentSensor {
public:
    void begin(int pin) { _pin = pin; pinMode(pin, INPUT); }
    float readAmps();
private:
    int _pin = -1;
};
