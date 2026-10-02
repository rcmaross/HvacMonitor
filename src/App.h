#pragma once

#include "TempSensor.h"

class App
{
public:
    void begin();
    void run();

private:
    static constexpr size_t MAX_TEMP_SENSORS = 8;

    TemperatureSensor* tempSensors[MAX_TEMP_SENSORS] = {};
    size_t tempSensorCount = 0;

    void addTemperatureSensor(
        const char* name,
        int pin
    );

    TemperatureSensor* getTemperatureSensor(const char *name);

};