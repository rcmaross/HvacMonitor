#pragma once

#include <lvgl.h>
#include "Status.h"
#include "ScreenUI.h"
#include "Network.h"
#include "Settings.h"
#include "Clock.h"

class UIManager
{
public:
    UIManager(Status& status, Network& network, Settings& settings, Clock& clock);
    ~UIManager();

    void update();

private:
    enum class Screen
    {
        Status,
        Settings,
        Network
    };

    Status& _status;
    Network& _network;
    Settings& _settings;
    Clock& _clock;
    
    lv_obj_t* _header = nullptr;
    lv_obj_t* _wifiBars[4] = {};
    lv_obj_t* _timeLabel = nullptr;
    lv_obj_t* _stateDot = nullptr;
    lv_obj_t* _efficiencyLabel = nullptr;

    lv_obj_t* _contentArea = nullptr;
    lv_obj_t* _tabBar = nullptr;

    lv_obj_t* _statusButton = nullptr;
    lv_obj_t* _settingsButton = nullptr;
    lv_obj_t* _networkButton = nullptr;

    ScreenUI* _currentUI = nullptr;

    Screen _currentScreen = Screen::Status;

    void createHeader();
    void updateHeader();

    void createNavigation();
    void showScreen(Screen screen);
    void clearCurrentScreen();

    static void statusButtonClicked(lv_event_t* event);
    static void settingsButtonClicked(lv_event_t* event);
    static void networkButtonClicked(lv_event_t* event);
};