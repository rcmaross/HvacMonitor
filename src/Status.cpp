#include "Status.h"

Status::Status(
    TemperatureSensor* largePipe,
    TemperatureSensor* smallPipe,
    TemperatureSensor* outdoor
)
    : _largePipeSensor(largePipe),
      _smallPipeSensor(smallPipe),
      _outdoorSensor(outdoor)
{
}

void Status::update()
{
    static uint32_t _lastUpdate = 0;
    uint32_t now = millis();

    if (_lastUpdate != 0 &&
        now - _lastUpdate < UPDATE_INTERVAL_MS)
    {
        return;
    }

    _lastUpdate = now;

    _largePipeTemperature =
        _largePipeSensor->readFahrenheit();

    _smallPipeTemperature =
        _smallPipeSensor->readFahrenheit();

    _outdoorTemperature =
        _outdoorSensor->readFahrenheit();
}