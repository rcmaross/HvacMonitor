#include "Settings.h"

void Settings::begin()
{
    Preferences prefs;

    if (!prefs.begin("settings", true))
        return;

    _ntpEnabled = prefs.getBool("ntp", true);
    _useMetric = prefs.getBool("metric", false);
    _zipcode = prefs.getString("zipcode", "");
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

bool Settings::setZipcode(const String& zipcode)
{
    if (zipcode.length() != 5) return false;
    for (size_t i = 0; i < 5; ++i)
        if (zipcode[i] < '0' || zipcode[i] > '9') return false;
    if (_zipcode != zipcode) { _zipcode = zipcode; save(); }
    return true;
}

void Settings::save()
{
    Preferences prefs;

    if (!prefs.begin("settings", false))
        return;

    prefs.putBool("ntp", _ntpEnabled);
    prefs.putBool("metric", _useMetric);
    prefs.putString("zipcode", _zipcode);
    prefs.putString("timezone", _timezone);

    prefs.end();
}