#include "Analysis.h"
#include <esp_heap_caps.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>

Analysis::Analysis(History& history) : _history(history) {}
Analysis::~Analysis() { free(_events); }

void Analysis::save() {
    Preferences p;
    p.begin("analysis", false);
    p.putBool("auto", _auto);
    p.putInt("low", _low);
    p.putInt("high", _high);
    p.putInt("ignore", _ignoreOff);
    p.end();
}

void Analysis::begin() {
    Preferences p;
    p.begin("analysis", true);
    _auto = p.getBool("auto", true);
    _low = p.getInt("low", 7);
    _high = p.getInt("high", 17);
    _ignoreOff = p.getInt("ignore", 10);
    p.end();
    if (_low < 0 || _high > 100 || _low >= _high) { _low = 7; _high = 17; }
    if (_ignoreOff < 0 || _ignoreOff > 120 || _ignoreOff % 10) _ignoreOff = 10;
    _events = (AnalysisEvent*)heap_caps_malloc(CAPACITY * sizeof(AnalysisEvent), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!_events) _events = (AnalysisEvent*)malloc(CAPACITY * sizeof(AnalysisEvent));
    _cutoff = (uint32_t)time(nullptr);
    _loading = _events && _history.available();
}

void Analysis::setAuto(bool enabled) { _auto = enabled; save(); recalculate(); }
bool Analysis::setThresholds(int low, int high) {
    if (_auto || low < 0 || high > 100 || low >= high) return false;
    _low = low; _high = high; save(); return true;
}
void Analysis::setIgnoreOff(int seconds) {
    if (seconds < 0 || seconds > 120 || seconds % 10) return;
    _ignoreOff = seconds; save();
}

void Analysis::append(uint32_t start, uint32_t seconds, AnalysisEventType type, float delta, bool split) {
    if (!_events || !seconds) return;
    if (_count && !split) {
        auto& last = _events[(_head + _count - 1) % CAPACITY];
        if (last.type == type && last.start + last.seconds == start) {
            last.seconds += seconds;
            if (type == AnalysisEventType::HEATING) last.peakDeltaC = fmaxf(last.peakDeltaC, delta);
            return;
        }
    }
    if (_count == CAPACITY) { _head = (_head + 1) % CAPACITY; --_count; }
    _events[(_head + _count) % CAPACITY] = {start, seconds, delta, type};
    ++_count;
}

void Analysis::feed(uint32_t timestamp, CompressorState state, float delta) {
    if (!timestamp || !isfinite(delta) || state == CompressorState::UNKNOWN) return;
    auto type = state == CompressorState::OFF ? AnalysisEventType::OFF : AnalysisEventType::HEATING;
    if (_hasPrevious && timestamp > _previousTime && timestamp - _previousTime <= 30) {
        append(_previousTime, timestamp - _previousTime, _previousType, _previousDelta,
               _previousState == CompressorState::DOWNSTAGING && state == CompressorState::PLATEAU);
    }
    _previousTime = timestamp;
    _previousType = type;
    _previousState = state;
    _previousDelta = delta;
    _hasPrevious = true;
}

void Analysis::sample(const Status& status) {
    if (_loading || !_events) return;
    time_t now = time(nullptr);
    if (now < 1700000000) return;
    float large = status.largePipeTemperature();
    float small = status.smallPipeTemperature();
    if (!isfinite(large) || !isfinite(small)) return;
    feed((uint32_t)now, status.compressorState(), fabsf(large - small));
}

void Analysis::calibrate() {
    float centers[3] = {}, minimum = INFINITY, maximum = -INFINITY;
    size_t count = 0;
    for (size_t i = 0; i < _count; ++i) {
        auto& e = at(i);
        if (e.type != AnalysisEventType::HEATING) continue;
        minimum = fminf(minimum, e.peakDeltaC);
        maximum = fmaxf(maximum, e.peakDeltaC);
        ++count;
    }
    if (count < 6 || maximum - minimum < 4) return;
    centers[0] = minimum; centers[1] = (minimum + maximum) / 2; centers[2] = maximum;
    for (int iteration = 0; iteration < 20; ++iteration) {
        double sum[3] = {}, weight[3] = {};
        for (size_t i = 0; i < _count; ++i) {
            auto& e = at(i);
            if (e.type != AnalysisEventType::HEATING) continue;
            int best = 0;
            for (int k = 1; k < 3; ++k)
                if (fabsf(e.peakDeltaC - centers[k]) < fabsf(e.peakDeltaC - centers[best])) best = k;
            sum[best] += e.peakDeltaC * e.seconds;
            weight[best] += e.seconds;
        }
        if (!weight[0] || !weight[1] || !weight[2]) return;
        for (int k = 0; k < 3; ++k) centers[k] = sum[k] / weight[k];
    }
    int low = roundf((centers[0] + centers[1]) / 2);
    int high = roundf((centers[1] + centers[2]) / 2);
    if (low >= 0 && high <= 100 && high - low >= 2) { _low = low; _high = high; }
}
