#pragma once

#include <Preferences.h>

class Settings
{
public:
    void begin();

    bool ntpEnabled() const { return _ntpEnabled; }
    bool useMetric() const { return _useMetric; }

    void setNtpEnabled(bool enabled);
    void setUseMetric(bool metric);

    const String& timezone() const { return _timezone; }
    void setTimezone(const String& timezone);

private:
    bool _ntpEnabled = true;
    bool _useMetric = false;
    String _timezone = "EST5EDT,M3.2.0,M11.1.0";

    void save();
};