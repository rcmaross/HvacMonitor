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
    _largePipeTemperature =
        _largePipeSensor->readFahrenheit();

    _smallPipeTemperature =
        _smallPipeSensor->readFahrenheit();

    _outdoorTemperature =
        _outdoorSensor->readFahrenheit();
}