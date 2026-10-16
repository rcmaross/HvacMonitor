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
    _cutoff = 0;
    _loading = _events && _history.available();
    Serial.printf("Analysis: event size=%u, capacity=%u\n",
                (unsigned)sizeof(AnalysisEvent),
                (unsigned)CAPACITY);
    Serial.printf("Analysis: events=%p, free PSRAM=%u\n",
                (void*)_events,
                (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    Serial.printf("Analysis: SD available=%d, loading=%d\n",
                _history.available(), _loading);
}

void Analysis::setAuto(bool enabled) { _auto = enabled; save(); recalculate(); }
bool Analysis::setThresholds(int low, int high) {
    if (_auto || low < 0 || high > 100 || low >= high) return false;
    _low = low; _high = high; save(); return true;
}
bool Analysis::setStrengthThresholds(int low, int high) {
    if (low < 0 || high > 100 || low >= high) return false;
    // The public strength scale maps to the current 0..50 C delta range.
    // Keep the conversion here so the UI remains independent of the algorithm.
    int internalLow = (low + 1) / 2;
    int internalHigh = (high + 1) / 2;
    if (internalLow >= internalHigh) return false;
    return setThresholds(internalLow, internalHigh);
}
void Analysis::setIgnoreOff(int seconds) {
    if (seconds < 0 || seconds > 120 || seconds % 10) return;
    _ignoreOff = seconds; save();
}

void Analysis::append(uint32_t start, uint32_t seconds, AnalysisEventType type, float avg) {
    if (!_events || !seconds) return;
    if (_count == CAPACITY) { _head = (_head + 1) % CAPACITY; --_count; }
    _events[(_head + _count) % CAPACITY] = {start, seconds, avg, type};
    ++_count;
}

void Analysis::finish(uint32_t end, const char* reason) {
    if (!_hasPrevious || !_sampleCount || end <= _intervalStart) return;
    uint32_t duration = end - _intervalStart;
    float avg = _sumDelta / _sampleCount;
    append(_intervalStart, duration, _previousType, avg);
    _completed = {true, _intervalStart, duration, _sampleCount, avg, _previousType, reason};
    _sumDelta = 0;
    _sampleCount = 0;
}

// Each incoming sample belongs to the interval beginning at its timestamp.
// The previous sample's state accounts for elapsed time until this sample.
void Analysis::feed(uint32_t timestamp, CompressorState state, float delta) {
    _completed.valid = false;
    if (!timestamp || !isfinite(delta) || state == CompressorState::UNKNOWN) {
        _hasPrevious = false;
        _sampleCount = 0;
        _sumDelta = 0;
        return;
    }
    AnalysisEventType type = state == CompressorState::OFF ? AnalysisEventType::OFF : AnalysisEventType::HEATING;
    if (_hasPrevious) {
        if (timestamp <= _previousTime || timestamp - _previousTime > 30) {
            finish(_previousTime + 10, "STATE_CHANGE");
            _hasPrevious = false;
        } else if (type != _previousType) {
            finish(timestamp, "STATE_CHANGE");
            _hasPrevious = false;
        } else if (timestamp >= _intervalStart + 60) {
            finish(_intervalStart + 60, "MINUTE");
            _hasPrevious = false;
        }
    }
    if (!_hasPrevious) {
        _intervalStart = timestamp;
        _sumDelta = 0;
        _sampleCount = 0;
    }
    _sumDelta += delta;
    if (_sampleCount < 255) ++_sampleCount;
    _previousTime = timestamp;
    _previousType = type;
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
    uint32_t now = (uint32_t)time(nullptr);
    uint32_t earliest = now > 86400 ? now - 86400 : 0;
    for (size_t i = 0; i < _count; ++i) {
        auto& e = at(i);
        if (e.type != AnalysisEventType::HEATING || e.start + e.seconds <= earliest) continue;
        minimum = fminf(minimum, e.avgDeltaC);
        maximum = fmaxf(maximum, e.avgDeltaC);
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
                if (fabsf(e.avgDeltaC - centers[k]) < fabsf(e.avgDeltaC - centers[best])) best = k;
            sum[best] += e.avgDeltaC * e.seconds;
            weight[best] += e.seconds;
        }
        if (!weight[0] || !weight[1] || !weight[2]) return;
        for (int k = 0; k < 3; ++k) centers[k] = sum[k] / weight[k];
    }
    int low = roundf((centers[0] + centers[1]) / 2);
    int high = roundf((centers[1] + centers[2]) / 2);
    if (low >= 0 && high <= 100 && high - low >= 2) { _low = low; _high = high; }
}

void Analysis::recalculate() {
    if (_loading || !_events) return;
    uint32_t now = (uint32_t)time(nullptr);
    if (now < 1700000000) return;
    uint32_t windowStart = now > 86400 ? now - 86400 : 0;
    if (_auto) calibrate();
    double off = 0, low = 0, medium = 0, high = 0;
    for (size_t i = 0; i < _count; ++i) {
        const auto& e = at(i);
        uint32_t start = e.start > windowStart ? e.start : windowStart;
        uint32_t end = e.start + e.seconds;
        if (end > now) end = now;
        if (end <= start) continue;
        bool heating = e.type == AnalysisEventType::HEATING;
        float avg = e.avgDeltaC;
        if (!heating && e.seconds <= (uint32_t)_ignoreOff && i > 0 && i + 1 < _count &&
            at(i - 1).type == AnalysisEventType::HEATING &&
            at(i + 1).type == AnalysisEventType::HEATING) {
            heating = true;
            avg = (at(i - 1).avgDeltaC + at(i + 1).avgDeltaC) / 2;
        }
        if (!heating) off += end - start;
        else if (avg <= _low) low += end - start;
        else if (avg <= _high) medium += end - start;
        else high += end - start;
    }
    double on = low + medium + high;
    double total = off + on;
    _off = total ? 100 * off / total : 0;
    _on = total ? 100 * on / total : 0;
    _lowPct = on ? 100 * low / on : 0;
    _mediumPct = on ? 100 * medium / on : 0;
    _highPct = on ? 100 * high / on : 0;
}
String Analysis::json() const {
    String s = "{";
    s += "\"loading\":" + String(_loading ? "true" : "false");
    s += ",\"auto\":" + String(_auto ? "true" : "false");
    s += ",\"low\":" + String(lowStrength()) + ",\"high\":" + String(highStrength());
    s += ",\"ignoreOff\":" + String(_ignoreOff);
    s += ",\"off\":" + String(_off, 1) + ",\"on\":" + String(_on, 1);
    s += ",\"lowPercent\":" + String(_lowPct, 1);
    s += ",\"mediumPercent\":" + String(_mediumPct, 1);
    s += ",\"highPercent\":" + String(_highPct, 1) + "}";
    return s;
}
void Analysis::parseHistoryLine(const String& line) {
    if (!_headerChecked) {
        _headerChecked = true;
        int column = 0, pos = 0;
        while (pos <= line.length()) {
            int next = line.indexOf(',', pos);
            if (next < 0) next = line.length();
            String field = line.substring(pos, next);
            field.trim();
            if (field == "timestamp") _stampColumn = column;
            else if (field == "large") _largeColumn = column;
            else if (field == "small") _smallColumn = column;
            else if (field == "compressor_state") _stateColumn = column;
            ++column;
            pos = next + 1;
        }
        _headerValid = _stampColumn >= 0 && _largeColumn >= 0 &&
                       _smallColumn >= 0 && _stateColumn >= 0;
        return;
    }
    if (!_headerValid) return;
    String stamp, largeText, smallText, name;
    int col = 0, pos = 0;
    while (pos <= line.length()) {
        int next = line.indexOf(',', pos);
        if (next < 0) next = line.length();
        if (col == _stampColumn) stamp = line.substring(pos, next);
        if (col == _largeColumn) largeText = line.substring(pos, next);
        if (col == _smallColumn) smallText = line.substring(pos, next);
        if (col == _stateColumn) name = line.substring(pos, next);
        ++col;
        pos = next + 1;
    }
    stamp.trim(); name.trim(); largeText.trim(); smallText.trim();
    // Completed summary rows in v4 have empty raw temperatures.
    if (!largeText.length() || !smallText.length()) return;
    struct tm tmv = {};
    if (!strptime(stamp.c_str(), "%Y-%m-%d %H:%M:%S", &tmv)) return;
    time_t t = mktime(&tmv);
    if (t <= 0 || (uint32_t)t > _cutoff || (uint32_t)t + 86400 < _cutoff) return;
    CompressorState state = CompressorState::UNKNOWN;
    if (name == "OFF") state = CompressorState::OFF;
    else if (name == "HEATING") state = CompressorState::HEATING;
    else if (name == "PLATEAU") state = CompressorState::PLATEAU;
    else if (name == "DOWNSTAGING") state = CompressorState::DOWNSTAGING;
    float delta = fabsf(largeText.toFloat() - smallText.toFloat());
    if (!_history.csvMetric()) delta *= 5.0f / 9.0f;
    feed((uint32_t)t, state, delta);
}

void Analysis::update() {
    if (_loading) {
        if (_cutoff == 0) {
            struct tm timeInfo;
            if (!getLocalTime(&timeInfo, 0)) return;
            _cutoff = static_cast<uint32_t>(time(nullptr));
            if (!_history.selectAnalysisFile(_cutoff)) {
                _loading = false;
                recalculate();
                return;
            }
        }
        bool done = false;
        bool ok = _history.readAnalysisBatch(_cursor, 150,
            [this](const String& line) { parseHistoryLine(line); }, done);
        if (!ok || done || (_headerChecked && !_headerValid)) {
            _loading = false;
            _hasPrevious = false;
            _sampleCount = 0;
            _sumDelta = 0;
            _completed.valid = false;
            recalculate();
        }
        return;
    }
    if (millis() - _lastRecalc >= 60000) {
        _lastRecalc = millis();
        recalculate();
    }
}
