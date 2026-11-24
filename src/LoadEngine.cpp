#include "LoadEngine.h"
void LoadEngine::reset() {
    _active = _hasCompleted = false;
    _start = _last = _samples = _eligible = 0;
    _sumCurrent = _sumDelta = _sumLoad = _effCurrent = _effDelta = _sumOutdoor = 0;
    _outdoorSamples = 0;
}
float LoadEngine::loadPercent(float current, const LoadConfig& c) {
    if (!isfinite(current) || current <= c.offThresholdA || c.referenceW <= 0 || c.voltageV <= 0) return 0;
    float value = 100.0f * current * c.voltageV / c.referenceW;
    return value > 100 ? 100 : value;
}
LoadEvent LoadEngine::currentEvent() const {
    LoadEvent e;
    if (!_active || !_samples) return e;
    e.start = _start;
    e.seconds = _last + 10 - _start;
    e.avgCurrentA = (float)(_sumCurrent / _samples);
    e.avgDeltaC = (float)(_sumDelta / _samples);
    e.loadPct = (float)(_sumLoad / _samples);
    e.fullLoadMinutes = (float)(_sumLoad * (10.0 / 6000.0));
    e.efficiencySeconds = _eligible * 10;
    if (_effDelta > 0 && _effCurrent > 0) {
        e.efficiencyIndex = (float)(_effCurrent / _effDelta);
        e.performanceScore = (float)(_effDelta / _effCurrent);
        e.scoreAvgCurrentA = (float)(_effCurrent / _eligible);
        e.scoreAvgDeltaC = (float)(_effDelta / _eligible);
    }
    if (_outdoorSamples) e.avgOutdoorC = (float)(_sumOutdoor / _outdoorSamples);
    return e;
}
void LoadEngine::finish() {
    LoadEvent e = currentEvent();
    if (e.seconds >= _config.minEventSeconds) {
        e.complete = true;
        _completed = e;
        _hasCompleted = true;
    }
    _active = false;
    _samples = _eligible = 0;
    _sumCurrent = _sumDelta = _sumLoad = _effCurrent = _effDelta = 0;
}
bool LoadEngine::takeCompleted(LoadEvent& event) {
    if (!_hasCompleted) return false;
    event = _completed;
    _hasCompleted = false;
    return true;
}
LoadSample LoadEngine::feed(uint32_t timestamp, float current, float large, float small, float outdoor) {
    LoadSample out;
    if (!timestamp || !isfinite(current) || current < 0) return out;
    if (_active && (timestamp <= _last || timestamp - _last > 30)) finish();
    out.running = current > _config.offThresholdA;
    if (!out.running) {
        if (_active) finish();
        return out;
    }
    if (!_active) {
        _active = true;
        _start = timestamp;
        _samples = _eligible = 0;
        _sumCurrent = _sumDelta = _sumLoad = _effCurrent = _effDelta = 0;
    }
    _last = timestamp;
    const float delta = isfinite(large) && isfinite(small) ? fabsf(large-small) : NAN;
    out.loadPct = loadPercent(current, _config);
    if (isfinite(outdoor)) { _sumOutdoor += outdoor; ++_outdoorSamples; }
    _sumCurrent += current;
    _sumLoad += out.loadPct;
    if (isfinite(delta)) _sumDelta += delta;
    ++_samples;
    if (timestamp - _start >= _config.startupSeconds &&
        isfinite(delta) && delta >= _config.minDeltaC) {
        _effCurrent += current;
        _effDelta += delta;
        ++_eligible;
        out.efficiencyIndex = current / delta;
    }
    return out;
}
