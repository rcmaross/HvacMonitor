#pragma once

#include "TempSensor.h"
#include "LVGLManager.h"
#include "Status.h"
#include "UIManager.h"

class App
{
public:
    void begin();
    void run();
    void printMemoryStats();

private:
    static constexpr size_t MAX_TEMP_SENSORS = 8;
    String tz_posix_rule = "EST5EDT,M3.2.0,M11.1.0";

    TemperatureSensor* tempSensors[MAX_TEMP_SENSORS] = {};
    size_t tempSensorCount = 0;

    TemperatureSensor* addTemperatureSensor(
        const char* name,
        int pin
    );

    TemperatureSensor* getTemperatureSensor(const char *name);

    LVGLManager _lvgl;

    Status* _status = nullptr;
    UIManager* _ui = nullptr;
};