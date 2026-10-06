#pragma once

#include <Arduino.h>

#include "Settings.h"
#include "Network.h"

class Clock
{
public:
    Clock(Settings& settings, Network& network);

    void begin();
    void update();

    void setLocalDateTime(
        int year,
        int month,
        int day,
        int hour,
        int minute
    );
    void timezoneChanged();
private:
    Settings& _settings;
    Network& _network;

    bool _ntpStarted = false;
    bool _ntpSynced = false;

    void initializeTimeFromRTC();
    void updateRTCFromSystemTime();
};