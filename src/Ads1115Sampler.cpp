#include "Ads1115Sampler.h"
#include <Wire.h>

Ads1115Sampler* Ads1115Sampler::_instance = nullptr;

void IRAM_ATTR Ads1115Sampler::onReady() {
    if (_instance) _instance->_ready = true;
}

bool Ads1115Sampler::begin() {
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(100000);
    pinMode(READY_PIN, INPUT); // GPIO34 requires external pull-up.
    _instance = this;
    attachInterrupt(digitalPinToInterrupt(READY_PIN), onReady, FALLING);
    _connected = _ads.begin(ADDRESS, &Wire);
    if (_connected) {
        _ads.setGain(GAIN_ONE);
        _ads.setDataRate(RATE_ADS1115_475SPS);
    }
    _nextSlot = millis();
    _lastRetry = millis();
    Serial.printf("ADS1115: SDA=%d SCL=%d ALERT=%d connected=%d\n",
                  SDA_PIN, SCL_PIN, READY_PIN, _connected);
    return _connected;
}

bool Ads1115Sampler::latest(uint8_t channel, int16_t& raw, uint32_t& sequence) const {
    if (channel >= 4 || _sequence[channel] == 0) return false;
    raw = _raw[channel];
    sequence = _sequence[channel];
    return true;
}

void Ads1115Sampler::update() {
    const uint32_t now = millis();
    if (!_connected) {
        if (now - _lastRetry < 2000) return;
        _lastRetry = now;
        _connected = _ads.begin(ADDRESS, &Wire);
        if (!_connected) return;
        _ads.setGain(GAIN_ONE);
        _ads.setDataRate(RATE_ADS1115_475SPS);
        _nextSlot = now;
    }

    if (_pending) {
        if (_ready || digitalRead(READY_PIN) == LOW) {
            _ready = false;
            _raw[_pendingChannel] = _ads.getLastConversionResults();
            ++_sequence[_pendingChannel];
            _pending = false;
        } else if (now - _started >= TIMEOUT_MS) {
            ++_timeouts;
            _pending = false;
            Serial.printf("ADS1115 ALERT timeout A%u\n", _pendingChannel);
        }
    }

    if (!_pending && static_cast<int32_t>(now - _nextSlot) >= 0) {
        _pendingChannel = CHANNELS[_slot];
        _ready = false;
        _ads.startADCReading(ADS1X15_REG_CONFIG_MUX_SINGLE_0 +
                            (static_cast<uint16_t>(_pendingChannel) << 12), false);
        _started = millis();
        _pending = true;
        _slot = (_slot + 1) % 3;
        _nextSlot += SLOT_MS;
        if (static_cast<int32_t>(now - _nextSlot) >= 0)
            _nextSlot = now + SLOT_MS;
    }
}
