#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "Status.h"
#include "History.h"
#include "AnalysisEngine.h"

class Analysis {
public:
    explicit Analysis(History& history);
    ~Analysis();
    void begin();
    void update();
    void sample(const Status& status);
    struct CompletedInterval {
        bool valid = false;
        uint32_t start = 0;
        uint32_t seconds = 0;
        uint8_t sampleCount = 0;
        float avgDeltaC = 0;
        AnalysisEventType type = AnalysisEventType::OFF;
        const char* reason = "IN_PROGRESS";
    };
    const CompletedInterval& completed() const { return _completed; }
    uint8_t sampleCount() const { return _sampleCount; }
    float runningAverage() const { return _sampleCount ? _sumDelta / _sampleCount : 0; }
    void recalculate();
    bool loading() const { return _loading; }
    bool autoMode() const { return _auto; }
    int low() const { return _low; }
    int lowStrength() const { return _low; }
    int highStrength() const { return _high; }
    int high() const { return _high; }
    int ignoreOff() const { return _ignoreOff; }
    void setAuto(bool enabled);
    bool setThresholds(int low, int high);
    bool setStrengthThresholds(int low, int high);
    void setIgnoreOff(int seconds);
    String json() const;
private:
    static constexpr size_t CAPACITY = 12000;
    History& _history;
    AnalysisEvent* _events = nullptr;
    size_t _head = 0, _count = 0;
    bool _loading = false, _auto = true, _hasPrevious = false;
    int _low = 6, _high = 40, _ignoreOff = 10; // 30-second ticks: MAINT <=3m, HIGH >=20m
    float _off = 0, _on = 0, _lowPct = 0, _mediumPct = 0, _highPct = 0;
    uint32_t _cutoff = 0, _cursor = 0, _previousTime = 0, _lastRecalc = 0;
    AnalysisEventType _previousType = AnalysisEventType::OFF;
    float _previousDelta = 0, _sumDelta = 0;
    uint8_t _sampleCount = 0;
    uint32_t _intervalStart = 0;
    CompletedInterval _completed;
    void append(uint32_t start, uint32_t seconds, AnalysisEventType type, float avg);
    void finish(uint32_t end, const char* reason);
    void feed(uint32_t timestamp, CompressorState state, float delta);
    void calibrate();
    int _stampColumn = -1, _largeColumn = -1, _smallColumn = -1, _outdoorColumn = -1, _stateColumn = -1;
    bool _headerChecked = false, _headerValid = false;
    void parseHistoryLine(const String& line);
    void resetHistoryHeader();
    uint32_t _lastHistoryTimestamp = 0;
    uint32_t _lastLiveTimestamp = 0;
    void save();
    const AnalysisEvent& at(size_t i) const { return _events[(_head + i) % CAPACITY]; }
};
