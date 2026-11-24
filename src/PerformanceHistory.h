#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <stdint.h>
#include "LoadEngine.h"

// Persistent event-count-weighted performance history.
// UTC days and weeks; Sunday is day 0. Files live alongside diagnostic history.
class PerformanceHistory {
public:
    void begin();
    void update(uint32_t nowUtc);
    void record(const LoadEvent& event);
private:
    struct Bin {
        int16_t outdoorBinC = 0;
        uint8_t mode = 0; // 0 = heat, 1 = cool
        uint32_t count = 0;
        double scoreSum = 0;
    };
    static constexpr size_t MAX_BINS = 32;
    Bin _bins[MAX_BINS] = {};
    uint8_t _count = 0;
    uint32_t _day = 0;
    bool _ready = false;
    static uint32_t dayStart(uint32_t utc) { return utc - utc % 86400U; }
    static uint32_t weekStart(uint32_t utc) { return dayStart(utc) - ((dayStart(utc) / 86400U + 4U) % 7U) * 86400U; }
    static String iso(uint32_t utc);
    static String dailyName(uint32_t day);
    void restore();
    void persist();
    bool flushDay();
    void consolidate(uint32_t currentWeek);
};
