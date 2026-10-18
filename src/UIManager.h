#pragma once

#include <lvgl.h>
#include "Status.h"
#include "ScreenUI.h"
#include "Network.h"
#include "Settings.h"
#include "Clock.h"
#include "Analysis.h"

class UIManager
{
public:
    UIManager(Status& status, Network& network, Settings& settings, Clock& clock, Analysis& analysis);
    ~UIManager();

    void update();

private:
    enum class Screen
    {
        Status,
        Analysis,
        Settings,
        Network,
        Firmware
    };

    Status& _status;
    Network& _network;
    Settings& _settings;
    Clock& _clock;
    Analysis& _analysis;
    
    lv_obj_t* _header = nullptr;
    lv_obj_t* _versionLabel = nullptr;
    lv_obj_t* _wifiBars[4] = {};
    lv_obj_t* _timeLabel = nullptr;
    lv_obj_t* _stateDot = nullptr;
    lv_obj_t* _efficiencyLabel = nullptr;

    lv_obj_t* _contentArea = nullptr;
    lv_obj_t* _tabBar = nullptr;

    lv_obj_t* _statusButton = nullptr;
    lv_obj_t* _analysisButton = nullptr;
    lv_obj_t* _settingsButton = nullptr;
    lv_obj_t* _networkButton = nullptr;
    lv_obj_t* _firmwareButton = nullptr;

    ScreenUI* _currentUI = nullptr;

    Screen _currentScreen = Screen::Status;

    void createHeader();
    void updateHeader();

    void createNavigation();
    void showScreen(Screen screen);
    void clearCurrentScreen();

    static void statusButtonClicked(lv_event_t* event);
    static void analysisButtonClicked(lv_event_t* event);
    static void settingsButtonClicked(lv_event_t* event);
    static void networkButtonClicked(lv_event_t* event);
    static void firmwareButtonClicked(lv_event_t* event);
};