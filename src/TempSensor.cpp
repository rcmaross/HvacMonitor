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

    float temperature = readInstantCelsius();

    for (int i = 0; i < SAMPLE_COUNT; i++)
        _samples[i] = temperature;

    _sampleIndex = 0;
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
    return readSampledCelsius();
}

float TemperatureSensor::readInstantCelsius()
{
    return resistanceToCelsius(readResistance());
}

float TemperatureSensor::readSampledCelsius()
{
    float temperature = readInstantCelsius();

    if (!isnan(temperature))
    {
        _samples[_sampleIndex] = temperature;

        _sampleIndex++;

        if (_sampleIndex >= SAMPLE_COUNT)
            _sampleIndex = 0;
    }

    return calculateAverage();
}
float TemperatureSensor::readFahrenheit()
{
    float celsius = readCelsius();

    if (isnan(celsius))
        return NAN;

    return celsius * 9.0f / 5.0f + 32.0f;
}

float TemperatureSensor::calculateAverage()
{
    float sorted[SAMPLE_COUNT];

    memcpy(
        sorted,
        _samples,
        sizeof(sorted)
    );

    for (int i = 1; i < SAMPLE_COUNT; i++)
    {
        float value = sorted[i];
        int j = i - 1;

        while (j >= 0 && sorted[j] > value)
        {
            sorted[j + 1] = sorted[j];
            j--;
        }

        sorted[j + 1] = value;
    }

    float total = 0.0f;

    for (int i = TRIM_COUNT; i < SAMPLE_COUNT - TRIM_COUNT; i++)
        total += sorted[i];

    return total / (SAMPLE_COUNT - (TRIM_COUNT * 2));
}