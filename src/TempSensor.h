#pragma once

#include <Arduino.h>

class TemperatureSensor
{
public:
    TemperatureSensor(const char* name, int pin);

    float readCelsius();
    float readFahrenheit();

    const char* name() const { return _name; }
    int pin() const { return _pin; }

private:
    static constexpr int SAMPLE_COUNT = 60;
    static constexpr int TRIM_COUNT = 3;

    const char* _name;
    int _pin;

    float _samples[SAMPLE_COUNT];
    int _sampleIndex = 0;

    static constexpr float SERIES_RESISTOR     = 10000.0f;
    static constexpr float NOMINAL_RESISTANCE  = 10000.0f;
    static constexpr float NOMINAL_TEMPERATURE = 25.0f;
    static constexpr float BETA                = 3950.0f;

    void begin();

    float readResistance();
    float resistanceToCelsius(float resistance);
    float readInstantCelsius();
    float readSampledCelsius();
    float calculateAverage();
};