#pragma once

#include "TempSensor.h"

class Status
{
public:
    Status(
        TemperatureSensor* largePipe,
        TemperatureSensor* smallPipe,
        TemperatureSensor* outdoor
    );

    void update();

    float largePipeTemperature() const { return _largePipeTemperature; }
    float smallPipeTemperature() const { return _smallPipeTemperature; }
    float outdoorTemperature()   const { return _outdoorTemperature; }

private:
    static constexpr uint32_t UPDATE_INTERVAL_MS = 1000;

    TemperatureSensor* _largePipeSensor;
    TemperatureSensor* _smallPipeSensor;
    TemperatureSensor* _outdoorSensor;

    float _largePipeTemperature = NAN;
    float _smallPipeTemperature = NAN;
    float _outdoorTemperature   = NAN;
};