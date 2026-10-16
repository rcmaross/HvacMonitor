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
               _splitNext);
        _splitNext = _previousState == CompressorState::DOWNSTAGING && state == CompressorState::PLATEAU;
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
    uint32_t now = (uint32_t)time(nullptr);
    uint32_t earliest = now > 86400 ? now - 86400 : 0;
    for (size_t i = 0; i < _count; ++i) {
        auto& e = at(i);
        if (e.type != AnalysisEventType::HEATING || e.start + e.seconds <= earliest) continue;
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
        float peak = e.peakDeltaC;
        if (!heating && e.seconds <= (uint32_t)_ignoreOff && i > 0 && i + 1 < _count &&
            at(i - 1).type == AnalysisEventType::HEATING &&
            at(i + 1).type == AnalysisEventType::HEATING) {
            heating = true;
            peak = fmaxf(at(i - 1).peakDeltaC, at(i + 1).peakDeltaC);
        }
        if (!heating) off += end - start;
        else if (peak <= _low) low += end - start;
        else if (peak <= _high) medium += end - start;
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
void Analysis::update() {
    if (_loading) {
        bool done = false;
            if (_cutoff == 0) {
                struct tm timeInfo;

                if (!getLocalTime(&timeInfo, 0))
                    return;

                _cutoff = static_cast<uint32_t>(time(nullptr));
            }        
            bool ok = _history.readAnalysisBatch(_cursor, 150, [this](const String& line) {
            int comma = line.indexOf(',');
            if (comma < 0) return;
            String stamp = line.substring(0, comma);
            struct tm tmv = {};
            if (!strptime(stamp.c_str(), "%Y-%m-%d %H:%M:%S", &tmv)) return;
            time_t t = mktime(&tmv);
            static int debugCount = 0;
            if (debugCount++ < 5) {
                Serial.printf(
                    "Analysis CSV: stamp=%s, parsed=%ld, cutoff=%lu, age=%ld\n",
                    stamp.c_str(),
                    (long)t,
                    (unsigned long)_cutoff,
                    (long)_cutoff - (long)t
                );
            }
            if (t <= 0 || (uint32_t)t > _cutoff || (uint32_t)t + 86400 < _cutoff) return;
            int p = comma + 1, q = line.indexOf(',', p);
            if (q < 0) return;
            float large = line.substring(p, q).toFloat();
            p = q + 1; q = line.indexOf(',', p);
            if (q < 0) return;
            float small = line.substring(p, q).toFloat();
            String name = line.substring(line.lastIndexOf(',') + 1);
            name.trim();
            CompressorState state = CompressorState::UNKNOWN;
            if (name == "OFF") state = CompressorState::OFF;
            else if (name == "HEATING") state = CompressorState::HEATING;
            else if (name == "PLATEAU") state = CompressorState::PLATEAU;
            else if (name == "DOWNSTAGING") state = CompressorState::DOWNSTAGING;
            float delta = fabsf(large - small);
            if (!_history.csvMetric()) delta *= 5.0f / 9.0f;
            feed((uint32_t)t, state, delta);
        }, done);
        if (!ok || done) {
            Serial.printf(
                "Analysis: rebuild finished, ok=%d, done=%d, cursor=%lu, events=%u\n",
                ok,
                done,
                (unsigned long)_cursor,
                (unsigned)_count
            );
            _loading = false;
            _hasPrevious = false;
            _splitNext = false;
            recalculate();
        }
        return;
    }
    if (millis() - _lastRecalc >= 60000) {
        _lastRecalc = millis();
        recalculate();
    }
}
