#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "Status.h"
#include "History.h"

enum class AnalysisEventType : uint8_t { OFF, HEATING };

struct AnalysisEvent {
    uint32_t startTime;
    uint32_t durationSeconds;
    float maxDeltaC;
    AnalysisEventType type;
};

class Analysis {
public:
    explicit Analysis(History& history);
    ~Analysis();
    void begin();
    void update();
    void sample(const Status& status);
    bool loading() const { return _loading; }
    bool ready() const { return !_loading; }
    bool autoCalibrate() const { return _auto; }
    int lowBoundary() const { return _low; }
    int highBoundary() const { return _high; }
    int ignoreOffSeconds() const { return _ignoreOff; }
    void setAutoCalibrate(bool enabled);
    bool setBoundaries(int low, int high);
    void setIgnoreOffSeconds(int seconds);
    void recalculate();
    float offPercent() const { return _offPercent; }
    float onPercent() const { return _onPercent; }
    float lowPercent() const { return _lowPercent; }
    float mediumPercent() const { return _mediumPercent; }
    float highPercent() const { return _highPercent; }
    String json() const;
private:
    static constexpr uint32_t WINDOW = 86400;
    static constexpr size_t CAPACITY = 12000;
    History& _history;
    AnalysisEvent* _events = nullptr;
    size_t _head = 0, _count = 0;
    bool _loading = true, _auto = true;
    int _low = 7, _high = 17, _ignoreOff = 10;
    float _offPercent = 0, _onPercent = 0;
    float _lowPercent = 0, _mediumPercent = 0, _highPercent = 0;
    uint32_t _cutoff = 0, _cursor = 0, _previousTimestamp = 0;
    bool _havePrevious = false;
    AnalysisEventType _previousType = AnalysisEventType::OFF;
    float _previousDelta = 0;
    CompressorState _previousState = CompressorState::UNKNOWN;
    bool _liveStarted = false;
    uint32_t _lastRecalc = 0;
    void addInterval(uint32_t start, uint32_t seconds, AnalysisEventType type, float delta,
                     bool split = false);
    void feed(uint32_t timestamp, CompressorState state, float delta);
    void finishLoad();
    void autoThresholds();
    void saveSettings();
    const AnalysisEvent& event(size_t i) const { return _events[(_head + i) % CAPACITY]; }
};
