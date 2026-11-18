#pragma once

#include <lvgl.h>

#include "ScreenUI.h"
#include "Settings.h"
#include "Clock.h"
#include "TextEntryUI.h"

class SettingsUI : public ScreenUI
{
public:
    SettingsUI(
        lv_obj_t* parent,
        Settings& settings,
        Clock& clock
    );

    virtual ~SettingsUI() override;

    void update() override;

private:
    Settings& _settings;
    Clock& _clock;

    lv_obj_t* _root = nullptr;

    lv_obj_t* _dateButton = nullptr;
    lv_obj_t* _dateLabel = nullptr;

    lv_obj_t* _timeButton = nullptr;
    lv_obj_t* _timeLabel = nullptr;

    lv_obj_t* _ntpSwitch = nullptr;

    lv_obj_t* _timezoneButton = nullptr;
    lv_obj_t* _timezoneLabel = nullptr;

    lv_obj_t* _unitsButton = nullptr;
    lv_obj_t* _unitsLabel = nullptr;
    lv_obj_t* _zipButton = nullptr;
    lv_obj_t* _zipLabel = nullptr;
    TextEntryUI* _zipEditor = nullptr;
    static void zipcodeClicked(lv_event_t* event);
    static void zipcodeEntered(const String& text, void* userData);
    static void zipcodeCancelled(void* userData);
    void closeZipEditor();

    void createLabel(const char* text, int y);
    lv_obj_t* createButton(int y, int width);

    void updateDateTime();
    void updateControls();

    static void ntpChanged(lv_event_t* event);
    static void timezoneClicked(lv_event_t* event);
    static void unitsClicked(lv_event_t* event);
};