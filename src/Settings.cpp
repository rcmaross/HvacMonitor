#include "Settings.h"

void Settings::begin()
{
    Preferences prefs;

    if (!prefs.begin("settings", true))
        return;

    _ntpEnabled = prefs.getBool("ntp", true);
    _useMetric = prefs.getBool("metric", false);
    _timezone = prefs.getString("timezone", "EST5EDT,M3.2.0,M11.1.0");

    prefs.end();
}

void Settings::setTimezone(const String& timezone)
{
    if (_timezone == timezone)
        return;

    _timezone = timezone;
    save();
}

void Settings::setNtpEnabled(bool enabled)
{
    if (_ntpEnabled == enabled)
        return;

    _ntpEnabled = enabled;
    save();
}

void Settings::setUseMetric(bool metric)
{
    if (_useMetric == metric)
        return;

    _useMetric = metric;
    save();
}

void Settings::save()
{
    Preferences prefs;

    if (!prefs.begin("settings", false))
        return;

    prefs.putBool("ntp", _ntpEnabled);
    prefs.putBool("metric", _useMetric);
    prefs.putString("timezone", _timezone);

    prefs.end();
}