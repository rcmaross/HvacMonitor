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
    p.putInt("minDuration", _minDuration);
    p.end();
}

void Analysis::begin() {
    Preferences p;
    p.begin("analysis", true);
    _auto = p.getBool("auto", true);
    _low = p.getInt("low", 6);
    _high = p.getInt("high", 40);
    _minDuration = p.getInt("minDuration", 30);
    p.end();
    if (_low != 6) _low = 6;
    if (_high <= _low || _high > 100) _high = 40;
    if (_minDuration < 0 || _minDuration > 120 || _minDuration % 10) _minDuration = 30;
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
    if (_auto || low != 6 || high <= low || high > 100) return false;
    _high = high; save(); recalculate(); return true;
}
bool Analysis::setStrengthThresholds(int low, int high) {
    // Values are 30-second ticks (0..100 = 0..50 minutes).
    return setThresholds(low, high);
}
void Analysis::setMinDuration(int seconds) {
    if (seconds < 0 || seconds > 120 || seconds % 10) return;
    _minDuration = seconds; save(); recalculate();
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
    _completed.valid = true;
    _completed.start = _intervalStart;
    _completed.seconds = duration;
    _completed.sampleCount = _sampleCount;
    _completed.avgDeltaC = avg;
    _completed.type = _previousType;
    _completed.reason = reason;
    _sumDelta = 0;
    _sampleCount = 0;
}

// Each incoming sample belongs to the interval beginning at its timestamp.
// The previous sample's state accounts for elapsed time until this sample.
void Analysis::flushPending(bool confirmed) {
    for (uint8_t i = 0; i < _pendingCount; ++i)
        feedResolved(_pending[i].timestamp,
                     confirmed ? CompressorState::HEATING : CompressorState::OFF,
                     _pending[i].delta);
    _pendingCount = 0;
}

// The detector is stateless. Hold at most 30 seconds of weak evidence,
// then either backdate confirmed heating or treat the samples as OFF.
void Analysis::feed(uint32_t timestamp, CompressorState state, float delta) {
    if (!timestamp || !isfinite(delta) || state == CompressorState::UNKNOWN) {
        flushPending(false);
        feedResolved(timestamp, state, delta);
        return;
    }
    if (_pendingCount && (timestamp <= _pending[_pendingCount - 1].timestamp ||
                          timestamp - _pending[0].timestamp > 30))
        flushPending(false);
    if (state == CompressorState::SUSPECTED_HEATING) {
        if (_pendingCount < 4) _pending[_pendingCount++] = {timestamp, delta};
        return;
    }
    if (_pendingCount) flushPending(state == CompressorState::HEATING);
    feedResolved(timestamp, state, delta);
}

void Analysis::feedResolved(uint32_t timestamp, CompressorState state, float delta) {
    _completed.valid = false;
    if (!timestamp || !isfinite(delta) || state == CompressorState::UNKNOWN) {
        _hasPrevious = false;
        _sampleCount = 0;
        _sumDelta = 0;
        return;
    }
    AnalysisEventType type = AnalysisEventType::OFF;
    switch (state) {
        case CompressorState::HEATING: type = AnalysisEventType::HEATING; break;
        case CompressorState::STANDBY_HEATING: type = AnalysisEventType::STANDBY_HEATING; break;
        case CompressorState::PLATEAU: type = AnalysisEventType::PLATEAU; break;
        case CompressorState::DOWNSTAGING: type = AnalysisEventType::DOWNSTAGING; break;
        default: break;
    }
    if (_hasPrevious) {
        if (timestamp <= _previousTime || timestamp - _previousTime > 30) {
            finish(_previousTime + 10, "STATE_CHANGE");
            _hasPrevious = false;
        } else if (type != _previousType) {
            finish(timestamp, "STATE_CHANGE");
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
    // Six 10-second samples form a minute; the sixth is the MINUTE row.
    if (!_completed.valid && _sampleCount >= 6) {
        finish(timestamp + 10, "MINUTE");
        _hasPrevious = false;
    }
}

void Analysis::sample(const Status& status) {
    if (!_events) return;
    time_t now = time(nullptr);
    if (now < 1700000000) return;
    float large = status.largePipeTemperature();
    float small = status.smallPipeTemperature();
    if (!isfinite(large) || !isfinite(small) || !isfinite(status.outdoorTemperature())) return;
    if (_loading) return; // History is still being replayed; CSV retains samples.
    if ((uint32_t)now <= _cutoff || (uint32_t)now <= _lastLiveTimestamp) return;
    _lastLiveTimestamp = (uint32_t)now;
    feed((uint32_t)now, status.compressorState(), fabsf(large - status.outdoorTemperature()));
}

void Analysis::calibrate() {
    // The engine operates on a chronological snapshot of the ring buffer.
    if (!_events || !_count) return;
    AnalysisEvent* ordered = (AnalysisEvent*)malloc(_count * sizeof(AnalysisEvent));
    if (!ordered) return;
    for (size_t i = 0; i < _count; ++i) ordered[i] = at(i);
    AnalysisEngine::calibrate(ordered, _count, (uint32_t)_minDuration, _high);
    free(ordered);
}

void Analysis::recalculate() {
    if (_loading || !_events) return;
    uint32_t now = (uint32_t)time(nullptr);
    if (now < 1700000000) return;
    if (_auto) calibrate();
    AnalysisEvent* ordered = (AnalysisEvent*)malloc(_count * sizeof(AnalysisEvent));
    if (!ordered) return;
    for (size_t i = 0; i < _count; ++i) ordered[i] = at(i);
    AnalysisTotals totals = AnalysisEngine::calculate(
        ordered, _count, now - 86400, now, _high, (uint32_t)_minDuration);
    free(ordered);
    _off = totals.offPercent;
    _on = totals.onPercent;
    _lowPct = totals.lowPercent;
    _maintPct = totals.maintPercent;
    _highPct = totals.highPercent;
}
String Analysis::json() const {
    String s = "{";
    s += "\"loading\":" + String(_loading ? "true" : "false");
    s += ",\"auto\":" + String(_auto ? "true" : "false");
    s += ",\"low\":" + String(lowStrength()) + ",\"high\":" + String(highStrength());
    s += ",\"minDuration\":" + String(_minDuration);
    s += ",\"off\":" + String(_off, 1) + ",\"on\":" + String(_on, 1);
    s += ",\"lowPercent\":" + String(_lowPct, 1);
    s += ",\"maintPercent\":" + String(_maintPct, 1);
    s += ",\"highPercent\":" + String(_highPct, 1) + "}";
    return s;
}
void Analysis::resetHistoryHeader() {
    _headerChecked = false;
    _headerValid = false;
    _historyCelsius = false;
    _stampColumn = _largeColumn = _smallColumn = _outdoorColumn = _stateColumn = -1;
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
            else if (field == "large" || field == "large_c") {
                _largeColumn = column;
                if (field == "large_c") _historyCelsius = true;
            }
            else if (field == "small" || field == "small_c") _smallColumn = column;
            else if (field == "outside" || field == "outside_c") _outdoorColumn = column;
            else if (field == "compressor_state") _stateColumn = column;
            ++column;
            pos = next + 1;
        }
        _headerValid = _stampColumn >= 0 && _largeColumn >= 0 &&
                       _smallColumn >= 0 && _outdoorColumn >= 0 && _stateColumn >= 0;
        return;
    }
    if (!_headerValid) return;
    String stamp, largeText, smallText, outdoorText, name;
    int col = 0, pos = 0;
    while (pos <= line.length()) {
        int next = line.indexOf(',', pos);
        if (next < 0) next = line.length();
        if (col == _stampColumn) stamp = line.substring(pos, next);
        if (col == _largeColumn) largeText = line.substring(pos, next);
        if (col == _smallColumn) smallText = line.substring(pos, next);
        if (col == _outdoorColumn) outdoorText = line.substring(pos, next);
        if (col == _stateColumn) name = line.substring(pos, next);
        ++col;
        pos = next + 1;
    }
    stamp.trim(); name.trim(); largeText.trim(); smallText.trim(); outdoorText.trim();
    // Completed summary rows in v4 have empty raw temperatures.
    if (!largeText.length() || !smallText.length() || !outdoorText.length()) return;
    struct tm tmv = {};
    if (!strptime(stamp.c_str(), "%Y-%m-%d %H:%M:%S", &tmv)) return;
    time_t t = mktime(&tmv);
    if (t <= 0 || (uint32_t)t > _cutoff || (uint32_t)t + 86400 < _cutoff) return;
    // Older files may overlap newer files. Never replay a timestamp.
    if ((uint32_t)t <= _lastHistoryTimestamp) return;
    CompressorState state = CompressorState::UNKNOWN;
    if (name == "OFF") state = CompressorState::OFF;
    else if (name == "HEATING") state = CompressorState::HEATING;
    else if (name == "SUSPECTED_HEATING") state = CompressorState::SUSPECTED_HEATING;
    else if (name == "STANDBY_HEATING") state = CompressorState::STANDBY_HEATING;
    else if (name == "PLATEAU") state = CompressorState::PLATEAU;
    else if (name == "DOWNSTAGING") state = CompressorState::DOWNSTAGING;
    float delta = fabsf(largeText.toFloat() - outdoorText.toFloat());
    if (!_historyCelsius && !_history.csvMetric()) delta *= 5.0f / 9.0f;
    if (state == CompressorState::UNKNOWN) return;
    _lastHistoryTimestamp = (uint32_t)t;
    feed((uint32_t)t, state, delta);
}

void Analysis::update() {
    if (_loading) {
        if (_cutoff == 0) {
            struct tm timeInfo;
            if (!getLocalTime(&timeInfo, 0)) return;
            _cutoff = static_cast<uint32_t>(time(nullptr));
            if (!_history.selectAnalysisFiles(_cutoff)) {
                _loading = false;
                recalculate();
                return;
            }
        }
        bool done = false;
        bool ok = _history.readAnalysisBatch(_cursor, 150,
            [this](const String& line) {
                // Each file begins with a header. A header appearing mid-replay
                // resets the column map for that file.
                if (line.startsWith("timestamp,")) resetHistoryHeader();
                parseHistoryLine(line);
            }, done);
        if (!ok || done) {
            _loading = false;
            // Preserve the last partial interval for continuity with live data.
            _completed.valid = false;
            Serial.printf("Analysis: reconstruction complete, events=%u, last=%lu\n",
                          (unsigned)_count, (unsigned long)_lastHistoryTimestamp);
            recalculate();
        }
        return;
    }
    if (millis() - _lastRecalc >= 60000) {
        _lastRecalc = millis();
        recalculate();
    }
}
