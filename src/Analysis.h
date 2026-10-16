#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "Status.h"
#include "History.h"

enum class AnalysisEventType : uint8_t { OFF, HEATING };
struct AnalysisEvent {
    uint32_t start;
    uint32_t seconds;
    float peakDeltaC;
    AnalysisEventType type;
};

class Analysis {
public:
    explicit Analysis(History& history);
    ~Analysis();
    void begin();
    void update();
    void sample(const Status& status);
    void recalculate();
    bool loading() const { return _loading; }
    bool autoMode() const { return _auto; }
    int low() const { return _low; }
    int high() const { return _high; }
    int ignoreOff() const { return _ignoreOff; }
    void setAuto(bool enabled);
    bool setThresholds(int low, int high);
    void setIgnoreOff(int seconds);
    String json() const;
private:
    static constexpr size_t CAPACITY = 12000;
    History& _history;
    AnalysisEvent* _events = nullptr;
    size_t _head = 0, _count = 0;
    bool _loading = false, _auto = true, _hasPrevious = false;
    int _low = 7, _high = 17, _ignoreOff = 10;
    float _off = 0, _on = 0, _lowPct = 0, _mediumPct = 0, _highPct = 0;
    uint32_t _cutoff = 0, _cursor = 0, _previousTime = 0, _lastRecalc = 0;
    AnalysisEventType _previousType = AnalysisEventType::OFF;
    CompressorState _previousState = CompressorState::UNKNOWN;
    float _previousDelta = 0;
    void append(uint32_t start, uint32_t seconds, AnalysisEventType type, float delta, bool split);
    void feed(uint32_t timestamp, CompressorState state, float delta);
    void calibrate();
    void save();
    const AnalysisEvent& at(size_t i) const { return _events[(_head + i) % CAPACITY]; }
};
