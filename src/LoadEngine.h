#pragma once
#include <stdint.h>
#include <stddef.h>
#include <math.h>

// One record per electrical compressor run; no presentation-only event copies.
struct LoadEvent {
    uint32_t start = 0;
    uint32_t seconds = 0;
    float avgCurrentA = 0;
    float avgDeltaC = 0;
    float loadPct = 0;
    float fullLoadMinutes = 0;
    float efficiencyIndex = NAN; // A / C, lower is better
    uint32_t efficiencySeconds = 0;
    bool complete = false;
};
struct LoadConfig {
    float offThresholdA = 0.2f;
    float voltageV = 240.0f;
    float referenceW = 2400.0f; // provisional until user enters nameplate input
    uint32_t startupSeconds = 120;
    float minDeltaC = 2.0f;
    uint32_t minEventSeconds = 30;
};
struct LoadSample {
    float loadPct = NAN;
    float efficiencyIndex = NAN;
    bool running = false;
};
class LoadEngine {
public:
    explicit LoadEngine(LoadConfig config = {}) : _config(config) {}
    void configure(LoadConfig config) { _config = config; reset(); }
    void reset();
    LoadSample feed(uint32_t timestamp, float currentA, float largeC, float smallC);
    bool takeCompleted(LoadEvent& event);
    bool active() const { return _active; }
    LoadEvent currentEvent() const;
    static float loadPercent(float current, const LoadConfig& c);
private:
    LoadConfig _config;
    bool _active = false, _hasCompleted = false;
    uint32_t _start = 0, _last = 0, _samples = 0, _eligible = 0;
    double _sumCurrent = 0, _sumDelta = 0, _sumLoad = 0;
    double _effCurrent = 0, _effDelta = 0;
    LoadEvent _completed;
    void finish();
};
