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
        _largePipeSensor->readCelsius();

    _smallPipeTemperature =
        _smallPipeSensor->readCelsius();

    _outdoorTemperature =
        _outdoorSensor->readCelsius();
}
void Status::sampleCompressor()
{
    // Fixed Fahrenheit thresholds regardless of UI temperature units.
    const float largeF = _largePipeTemperature * 1.8f + 32.0f;
    const float smallF = _smallPipeTemperature * 1.8f + 32.0f;
    const unsigned long nowMs = millis();

    _largeSlopeFPerMin = NAN;
    _smallSlopeFPerMin = NAN;
    _compressorState = CompressorState::UNKNOWN;

    if (isfinite(largeF) && isfinite(smallF) && _sampleCount >= SLOPE_SAMPLES)
    {
        const TemperatureSample& old = _samples[_sampleIndex];
        const float elapsedMinutes = (nowMs - old.timestampMs) / 60000.0f;
        if (elapsedMinutes > 0.0f && isfinite(old.largeF) && isfinite(old.smallF))
        {
            _largeSlopeFPerMin = (largeF - old.largeF) / elapsedMinutes;
            _smallSlopeFPerMin = (smallF - old.smallF) / elapsedMinutes;
            const float deltaF = largeF - smallF;
            const float deltaSlope = _largeSlopeFPerMin - _smallSlopeFPerMin;

            constexpr float SLOPE_DEADBAND = 0.2f;
            constexpr float PLATEAU_DELTA_F = 20.0f;
            constexpr float DOWNSTAGE_DELTA_SLOPE = 5.0f;

            if (_largeSlopeFPerMin > SLOPE_DEADBAND)
                _compressorState = CompressorState::HEATING;
            else if (fabsf(_largeSlopeFPerMin) <= SLOPE_DEADBAND &&
                     deltaF > PLATEAU_DELTA_F)
                _compressorState = CompressorState::PLATEAU;
            else if (_largeSlopeFPerMin < -SLOPE_DEADBAND &&
                     deltaSlope > DOWNSTAGE_DELTA_SLOPE)
                _compressorState = CompressorState::DOWNSTAGING;
            else
                _compressorState = CompressorState::OFF;
        }
    }

    _samples[_sampleIndex] = {largeF, smallF, nowMs};
    _sampleIndex = (_sampleIndex + 1) % SLOPE_SAMPLES;
    if (_sampleCount < SLOPE_SAMPLES)
        ++_sampleCount;
}

const char* Status::compressorStateName(CompressorState state)
{
    switch (state)
    {
        case CompressorState::OFF: return "OFF";
        case CompressorState::HEATING: return "HEATING";
        case CompressorState::PLATEAU: return "PLATEAU";
        case CompressorState::DOWNSTAGING: return "DOWNSTAGING";
        default: return "UNKNOWN";
    }
}
