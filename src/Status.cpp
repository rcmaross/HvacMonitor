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
    const float largeC = _largePipeTemperature;
    const float smallC = _smallPipeTemperature;
    const unsigned long nowMs = millis();

    _largeSlopeCPerMin = NAN;
    _smallSlopeCPerMin = NAN;
    _compressorState = CompressorState::UNKNOWN;

    if (isfinite(largeC) && isfinite(smallC) && _sampleCount >= SLOPE_SAMPLES)
    {
        const TemperatureSample& old = _samples[_sampleIndex];
        const float elapsedMinutes = (nowMs - old.timestampMs) / 60000.0f;
        if (elapsedMinutes > 0.0f && isfinite(old.largeC) && isfinite(old.smallC))
        {
            _largeSlopeCPerMin = (largeC - old.largeC) / elapsedMinutes;
            _smallSlopeCPerMin = (smallC - old.smallC) / elapsedMinutes;
            const float deltaC = largeC - smallC;
            const float deltaSlope = _largeSlopeCPerMin - _smallSlopeCPerMin;

            constexpr float START_SLOPE_DEADBAND = 0.3f;
            constexpr float SLOPE_DEADBAND = 0.2f;
            constexpr float INIT_DELTA_C = 3.0f;
            constexpr float PLATEAU_DELTA_C = 11.0f;
            constexpr float DOWNSTAGE_DELTA_SLOPE = 3.0f;

            if (_largeSlopeCPerMin > START_SLOPE_DEADBAND ||
                (_largeSlopeCPerMin > SLOPE_DEADBAND && deltaC >= INIT_DELTA_C))
                _compressorState = CompressorState::HEATING;
            else if (fabsf(_largeSlopeCPerMin) <= SLOPE_DEADBAND &&
                     deltaC > PLATEAU_DELTA_C)
                _compressorState = CompressorState::PLATEAU;
            else if (_largeSlopeCPerMin < -SLOPE_DEADBAND &&
                     deltaSlope > DOWNSTAGE_DELTA_SLOPE)
                _compressorState = CompressorState::DOWNSTAGING;
            else
                _compressorState = CompressorState::OFF;
        }
    }

    _samples[_sampleIndex] = {largeC, smallC, nowMs};
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
