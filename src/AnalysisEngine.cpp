#include "AnalysisEngine.h"
#include <math.h>

AnalysisTotals AnalysisEngine::calculate(const AnalysisEvent* events, size_t count,
                                         uint32_t windowStart, uint32_t now,
                                         int highTicks, uint32_t ignoreOff) {
    AnalysisTotals t;
    if (!events || now <= windowStart || !validHighTicks(highTicks)) return t;
    forEachCycle(events, count, ignoreOff, [&](Cycle cycle) {
        const uint32_t start = cycle.start > windowStart ? cycle.start : windowStart;
        const uint32_t end = cycle.end < now ? cycle.end : now;
        if (end <= start) return;
        const uint64_t seconds = end - start;
        switch (classify(cycle.end - cycle.start, highTicks)) {
            case Level::MAINT: t.maint += seconds; break;
            case Level::LOW: t.low += seconds; break;
            case Level::HIGH: t.high += seconds; break;
        }
    });
    t.on = t.maint + t.low + t.high;
    const uint64_t window = now - windowStart;
    t.off = window > t.on ? window - t.on : 0;
    t.onPercent = 100.0f * (float)t.on / (float)window;
    t.offPercent = 100.0f * (float)t.off / (float)window;
    if (t.on > 0) {
        t.maintPercent = 100.0f * (float)t.maint / (float)t.on;
        t.lowPercent = 100.0f * (float)t.low / (float)t.on;
        t.highPercent = 100.0f * (float)t.high / (float)t.on;
    }
    return t;
}

bool AnalysisEngine::calibrate(const AnalysisEvent* events, size_t count,
                               uint32_t ignoreOff, int& highTicks) {
    if (!events) return false;
    uint32_t durations[256] = {};
    size_t n = 0;
    forEachCycle(events, count, ignoreOff, [&](Cycle cycle) {
        const uint32_t seconds = cycle.end - cycle.start;
        if (seconds > MAINT_SECONDS && n < 256) durations[n++] = seconds;
    });
    if (n < 4) return false;
    float shortCenter = 600, longCenter = 2100;
    for (int iteration = 0; iteration < 16; ++iteration) {
        uint64_t sumShort = 0, sumLong = 0;
        int countShort = 0, countLong = 0;
        for (size_t i = 0; i < n; ++i) {
            if (fabsf((float)durations[i] - shortCenter) <=
                fabsf((float)durations[i] - longCenter)) {
                sumShort += durations[i]; ++countShort;
            } else { sumLong += durations[i]; ++countLong; }
        }
        if (!countShort || !countLong) return false;
        shortCenter = (float)(sumShort / countShort);
        longCenter = (float)(sumLong / countLong);
    }
    if (longCenter - shortCenter < 300) return false;
    int ticks = (int)roundf((shortCenter + longCenter) / 60.0f);
    if (ticks > 100) ticks = 100;
    if (!validHighTicks(ticks)) return false;
    highTicks = ticks;
    return true;
}
