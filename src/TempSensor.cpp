#include "TempSensor.h"
#include <math.h>

TemperatureSensor::TemperatureSensor(const char * name, int pin)
    : _name(name), _pin(pin)
{
    begin();
}

void TemperatureSensor::begin()
{
    pinMode(_pin, INPUT);
}

float TemperatureSensor::readResistance()
{
    int adc = analogRead(_pin);

    if (adc <= 0 || adc >= 4095)
        return NAN;

    // Divider:
    // 3.3V -> 10K fixed -> ADC -> NTC -> GND
    //
    // Because we're using the ADC ratio directly, the 3.3V
    // supply voltage cancels out of the calculation.
    return SERIES_RESISTOR * adc / (4095.0f - adc);
}

float TemperatureSensor::resistanceToCelsius(float resistance)
{
    if (isnan(resistance))
        return NAN;

    // Beta equation for a 10K NTC thermistor
    float temperature =
        logf(resistance / NOMINAL_RESISTANCE) / BETA;

    temperature +=
        1.0f / (NOMINAL_TEMPERATURE + 273.15f);

    temperature = 1.0f / temperature;

    return temperature - 273.15f;
}

float TemperatureSensor::readCelsius()
{
    return resistanceToCelsius(readResistance());
}

float TemperatureSensor::readFahrenheit()
{
    float celsius = readCelsius();

    if (isnan(celsius))
        return NAN;

    return celsius * 9.0f / 5.0f + 32.0f;
}