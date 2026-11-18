#pragma once
#include <Arduino.h>
class OutdoorWeather {
public:
    void update(const String& zipcode, bool connected);
    float temperatureC() const { return _temperatureC; }
private:
    String _resolvedZip;
    String _requestedZip;
    float _lat = NAN, _lon = NAN, _temperatureC = NAN;
    uint32_t _lastAttempt = 0;
    bool _attempted = false;
    bool resolveZip(const String& zipcode);
    bool fetchTemperature();
};
