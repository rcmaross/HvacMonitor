#pragma once

#include "TempSensor.h"

enum class CompressorState { UNKNOWN, OFF, HEATING, STANDBY_HEATING, PLATEAU, DOWNSTAGING };

class Status
{
public:
    Status(
        TemperatureSensor* largePipe,
        TemperatureSensor* smallPipe,
        TemperatureSensor* outdoor
    );

    void begin() {} //place holder for now.
    void update();
    void sampleCompressor();
    CompressorState compressorState() const { return _compressorState; }
    bool compressorHeating() const {
        return _compressorState == CompressorState::HEATING ||
               _compressorState == CompressorState::STANDBY_HEATING ||
               _compressorState == CompressorState::PLATEAU ||
               _compressorState == CompressorState::DOWNSTAGING;
    }
    static const char* compressorStateName(CompressorState state);
    float largeSlopeCPerMin() const { return _largeSlopeCPerMin; }
    float smallSlopeCPerMin() const { return _smallSlopeCPerMin; }

    float largePipeTemperature() const { return _largePipeTemperature; }
    float smallPipeTemperature() const { return _smallPipeTemperature; }
    float outdoorTemperature()   const { return _outdoorTemperature; }

private:
    static constexpr int SLOPE_SAMPLES = 3;
    struct TemperatureSample {
        float largeC;
        float smallC;
        unsigned long timestampMs;
    };
    TemperatureSample _samples[SLOPE_SAMPLES] = {};
    int _sampleIndex = 0;
    int _sampleCount = 0;
    float _largeSlopeCPerMin = NAN;
    float _smallSlopeCPerMin = NAN;
    CompressorState _compressorState = CompressorState::UNKNOWN;

    TemperatureSensor* _largePipeSensor;
    TemperatureSensor* _smallPipeSensor;
    TemperatureSensor* _outdoorSensor;

    float _largePipeTemperature = NAN;
    float _smallPipeTemperature = NAN;
    float _outdoorTemperature   = NAN;
};