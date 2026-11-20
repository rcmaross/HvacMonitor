#pragma once
#include <Arduino.h>
#ifdef PIO_UNIT_TESTING
// Native test double: keeps the ADS1115 hardware library out of host builds.
class Ads1115Sampler {
public:
    bool begin() { return true; }
    void update() {}
    bool latest(uint8_t channel, int16_t& raw, uint32_t& sequence) const {
        if (channel >= 4 || !_valid[channel]) return false;
        raw = _raw[channel];
        sequence = _sequence[channel];
        return true;
    }
    void inject(uint8_t channel, int16_t raw) {
        if (channel >= 4) return;
        _raw[channel] = raw;
        _valid[channel] = true;
        ++_sequence[channel];
    }
    uint32_t timeouts() const { return 0; }
    bool connected() const { return true; }
private:
    int16_t _raw[4] = {};
    uint32_t _sequence[4] = {};
    bool _valid[4] = {};
};
#else
#include <Adafruit_ADS1X15.h>

class Ads1115Sampler {
public:
    bool begin();
    void update();
    bool latest(uint8_t channel, int16_t& raw, uint32_t& sequence) const;
    uint32_t timeouts() const { return _timeouts; }
    bool connected() const { return _connected; }
private:
    static constexpr int SDA_PIN = 32, SCL_PIN = 33, READY_PIN = 34;
    static constexpr uint8_t ADDRESS = 0x48;
    static constexpr uint32_t SLOT_MS = 333, TIMEOUT_MS = 30;
    static constexpr uint8_t CHANNELS[3] = {0, 1, 3};
    static Ads1115Sampler* _instance;
    static void IRAM_ATTR onReady();
    volatile bool _ready = false;
    Adafruit_ADS1115 _ads;
    bool _connected = false, _pending = false;
    uint8_t _slot = 0, _pendingChannel = 0;
    uint32_t _nextSlot = 0, _started = 0, _lastRetry = 0, _timeouts = 0;
    int16_t _raw[4] = {};
    uint32_t _sequence[4] = {};
};

#endif // PIO_UNIT_TESTING
