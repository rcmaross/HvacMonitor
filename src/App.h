#pragma once

#include "TempSensor.h"
#include "LVGLManager.h"
#include "Status.h"
#include "Network.h"
#include "Settings.h"
#include "Clock.h"
#include "UIManager.h"

class App
{
public:
    void begin();
    void run();
    void printMemoryStats();
    Settings& settings() { return *_settings; }

private:
    static constexpr uint32_t ONESECOND_INTERVAL_MS = 1000;
    static constexpr uint32_t QUARTERSECOND_INTERVAL_MS = 250;
    static constexpr size_t MAX_TEMP_SENSORS = 8;

    TemperatureSensor* tempSensors[MAX_TEMP_SENSORS] = {};
    size_t tempSensorCount = 0;
    void runEverySecond();
    void runEveryQuarterSecond();
    TemperatureSensor* addTemperatureSensor(
        const char* name,
        int pin
    );

    TemperatureSensor* getTemperatureSensor(const char *name);

    LVGLManager _lvgl;
    UIManager* _ui = nullptr;

    Status* _status = nullptr;
    Network* _network = nullptr;
    Settings* _settings = nullptr;
    Clock* _clock = nullptr;
};