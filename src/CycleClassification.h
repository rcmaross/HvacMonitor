#pragma once
#include <stdint.h>

// Pure functions shared by firmware and native unit tests.
namespace CycleClassification {
enum class Level : uint8_t { MAINT, LOW, HIGH };
constexpr uint32_t MAINT_SECONDS = 180;
constexpr int TICKS_PER_MINUTE = 2;
constexpr int MAX_TICKS = 100;
inline uint32_t secondsFromTicks(int ticks) { return ticks <= 0 ? 0u : static_cast<uint32_t>(ticks) * 30u; }
inline bool validHighTicks(int ticks) { return ticks > 6 && ticks <= MAX_TICKS; }
inline Level classify(uint32_t seconds, int highTicks) {
    if (seconds <= MAINT_SECONDS) return Level::MAINT;
    return seconds >= secondsFromTicks(highTicks) ? Level::HIGH : Level::LOW;
}
inline bool canBridgeOff(uint32_t offSeconds, uint32_t ignoreOffSeconds) {
    return offSeconds <= ignoreOffSeconds;
}
}
