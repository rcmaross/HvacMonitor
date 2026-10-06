#include "Clock.h"

#include <M5Unified.h>
#include <sys/time.h>
#include <time.h>

Clock::Clock(Settings& settings, Network& network)
    : _settings(settings), _network(network)
{
}

void Clock::begin()
{
    setenv("TZ", _settings.timezone().c_str(), 1);
    tzset();

    initializeTimeFromRTC();
}
void Clock::update()
{
    if (!_settings.ntpEnabled())
    {
        _ntpStarted = false;
        _ntpSynced = false;
        return;
    }

    if (!_network.isConnected())
        return;

    if (!_ntpStarted)
    {
        Serial.println("Starting NTP");

        configTzTime(
            _settings.timezone().c_str(),
            "pool.ntp.org",
            "time.nist.gov"
        );

        _ntpStarted = true;
        return;
    }

    if (_ntpSynced)
        return;

    struct tm timeInfo;

    if (!getLocalTime(&timeInfo, 0))
        return;

    //
    // Reject obviously bogus pre-NTP dates.
    //

    if (timeInfo.tm_year + 1900 < 2025)
        return;

    _ntpSynced = true;

    Serial.println("NTP synchronized");

    updateRTCFromSystemTime();
}

void Clock::initializeTimeFromRTC()
{
    if (!M5.Rtc.isEnabled())
    {
        Serial.println("RTC not available");
        return;
    }

    auto rtc = M5.Rtc.getDateTime();

    //
    // RTC is stored as UTC.
    //

    struct tm utc = {};

    utc.tm_year = rtc.date.year - 1900;
    utc.tm_mon  = rtc.date.month - 1;
    utc.tm_mday = rtc.date.date;

    utc.tm_hour = rtc.time.hours;
    utc.tm_min  = rtc.time.minutes;
    utc.tm_sec  = rtc.time.seconds;

    //
    // TZ temporarily must not affect conversion because RTC is UTC.
    //

    setenv("TZ", "UTC0", 1);
    tzset();

    time_t timestamp = mktime(&utc);
    
    // set the actual timezone by using the change function
    timezoneChanged();

    if (timestamp <= 0)
    {
        Serial.println("RTC time invalid");
        return;
    }

    struct timeval tv =
    {
        .tv_sec = timestamp,
        .tv_usec = 0
    };

    settimeofday(&tv, nullptr);

    Serial.println("System time initialized from RTC");
}

void Clock::timezoneChanged()
{
    setenv("TZ", _settings.timezone().c_str(), 1);
    tzset();

}

void Clock::updateRTCFromSystemTime()
{
    if (!M5.Rtc.isEnabled())
        return;

    time_t now = time(nullptr);

    struct tm utc;
    gmtime_r(&now, &utc);

    M5.Rtc.setDateTime(&utc);

    Serial.println("RTC updated from system time");
}

void Clock::setLocalDateTime(
    int year,
    int month,
    int day,
    int hour,
    int minute
)
{
    struct tm local = {};

    local.tm_year = year - 1900;
    local.tm_mon  = month - 1;
    local.tm_mday = day;

    local.tm_hour = hour;
    local.tm_min  = minute;
    local.tm_sec  = 0;

    //
    // Let the C library determine whether DST applies.
    //

    local.tm_isdst = -1;

    time_t timestamp = mktime(&local);

    if (timestamp <= 0)
        return;

    struct timeval tv =
    {
        .tv_sec = timestamp,
        .tv_usec = 0
    };

    settimeofday(&tv, nullptr);

    updateRTCFromSystemTime();

    Serial.printf(
        "Manual time set: %04d-%02d-%02d %02d:%02d\n",
        year,
        month,
        day,
        hour,
        minute
    );
}