#pragma once
#include <stddef.h>
#include <stdint.h>

// Hardware-independent calculation engine. No Arduino or device dependencies.
enum class AnalysisEventType : uint8_t { OFF, HEATING };
struct AnalysisEvent {
    uint32_t start;
    uint32_t seconds;
    float avgDeltaC;
    AnalysisEventType type;
};
struct AnalysisTotals {
    uint64_t off = 0, on = 0, maint = 0, low = 0, high = 0;
    float offPercent = 0, onPercent = 0;
    float maintPercent = 0, lowPercent = 0, highPercent = 0;
};
class AnalysisEngine {
public:
    static constexpr uint32_t MAINT_SECONDS = 180;
    static constexpr int DEFAULT_HIGH_TICKS = 40;
    static bool validHighTicks(int ticks) { return ticks > 6 && ticks <= 100; }
    static uint32_t secondsFromTicks(int ticks) { return ticks > 0 ? (uint32_t)ticks * 30u : 0u; }
    static bool canBridgeOff(uint32_t gap, uint32_t minDuration) { return gap <= minDuration; }
    enum class Level : uint8_t { Maint, Low, High };
    static Level classify(uint32_t duration, int highTicks) {
        if (duration <= MAINT_SECONDS) return Level::Maint;
        return duration >= secondsFromTicks(highTicks) ? Level::High : Level::Low;
    }
    // Events are in chronological order. The last cycle may still be in progress.
    static AnalysisTotals calculate(const AnalysisEvent* events, size_t count,
                                    uint32_t windowStart, uint32_t now,
                                    int highTicks, uint32_t minDuration);
    static bool calibrate(const AnalysisEvent* events, size_t count,
                          uint32_t minDuration, int& highTicks);
private:
    struct Cycle { uint32_t start, end; };
    template <typename F>
    static void forEachCycle(const AnalysisEvent* events, size_t count,
                             uint32_t minDuration, F callback) {
        bool active = false;
        Cycle cycle = {};
        for (size_t i = 0; i < count; ++i) {
            const AnalysisEvent& ev = events[i];
            if (!ev.seconds) continue;
            const uint32_t end = ev.start + ev.seconds;
            if (ev.type == AnalysisEventType::HEATING) {
                if (!active || (ev.start > cycle.end &&
                    !canBridgeOff(ev.start - cycle.end, minDuration))) {
                    if (active && cycle.end - cycle.start > minDuration) if (cycle.end - cycle.start > minDuration) callback(cycle);
                    cycle = {ev.start, end};
                    active = true;
                } else if (end > cycle.end) cycle.end = end;
            } else if (active && ev.seconds > minDuration) {
                if (cycle.end - cycle.start > minDuration) callback(cycle);
                active = false;
            }
        }
        if (active && cycle.end - cycle.start > minDuration) callback(cycle);
    }
};
