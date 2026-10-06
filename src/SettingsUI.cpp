#include "SettingsUI.h"

#include <time.h>

namespace
{
    constexpr int LEFT_X = 8;
    constexpr int VALUE_X = 118;
    constexpr int VALUE_WIDTH = 194;

    constexpr int DATE_Y = 4;
    constexpr int TIME_Y = 34;
    constexpr int NTP_Y = 64;
    constexpr int TIMEZONE_Y = 94;
    constexpr int UNITS_Y = 124;

    const lv_color_t BACKGROUND = lv_color_hex(0x101418);
    const lv_color_t PRIMARY_TEXT = lv_color_hex(0xF2F4F5);
}

SettingsUI::SettingsUI(
    lv_obj_t* parent,
    Settings& settings,
    Clock& clock
)
    : _settings(settings),
      _clock(clock)
{
    _root = lv_obj_create(parent);

    lv_obj_set_size(_root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(_root, BACKGROUND, 0);
    lv_obj_set_style_bg_opa(_root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_root, 0, 0);
    lv_obj_set_style_pad_all(_root, 0, 0);
    lv_obj_set_scrollable(_root, false);

    //
    // Date
    //

    createLabel("Date", DATE_Y + 7);

    _dateButton = createButton(DATE_Y, VALUE_WIDTH);

    _dateLabel = lv_label_create(_dateButton);
    lv_label_set_text(_dateLabel, "--/--/----");
    lv_obj_center(_dateLabel);

    //
    // Time
    //

    createLabel("Time", TIME_Y + 7);

    _timeButton = createButton(TIME_Y, VALUE_WIDTH);

    _timeLabel = lv_label_create(_timeButton);
    lv_label_set_text(_timeLabel, "--:-- --");
    lv_obj_center(_timeLabel);

    //
    // NTP
    //

    createLabel("NTP", NTP_Y + 5);

    _ntpSwitch = lv_switch_create(_root);
    lv_obj_set_pos(_ntpSwitch, VALUE_X, NTP_Y);
    lv_obj_add_event_cb(_ntpSwitch, ntpChanged, LV_EVENT_VALUE_CHANGED, this);

    //
    // Time zone
    //

    createLabel("Time Zone", TIMEZONE_Y + 7);

    _timezoneButton = createButton(TIMEZONE_Y, VALUE_WIDTH);

    _timezoneLabel = lv_label_create(_timezoneButton);
    lv_obj_center(_timezoneLabel);

    lv_obj_add_event_cb(
        _timezoneButton,
        timezoneClicked,
        LV_EVENT_CLICKED,
        this
    );

    //
    // Units
    //

    createLabel("Units", UNITS_Y + 7);

    _unitsButton = createButton(UNITS_Y, VALUE_WIDTH);

    _unitsLabel = lv_label_create(_unitsButton);
    lv_obj_center(_unitsLabel);

    lv_obj_add_event_cb(
        _unitsButton,
        unitsClicked,
        LV_EVENT_CLICKED,
        this
    );

    updateControls();
    updateDateTime();
}

SettingsUI::~SettingsUI()
{
    if (_root)
    {
        lv_obj_delete(_root);
        _root = nullptr;
    }
}

void SettingsUI::createLabel(const char* text, int y)
{
    lv_obj_t* label = lv_label_create(_root);

    lv_label_set_text(label, text);
    lv_obj_set_pos(label, LEFT_X, y);
    lv_obj_set_style_text_color(label, PRIMARY_TEXT, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
}

lv_obj_t* SettingsUI::createButton(int y, int width)
{
    lv_obj_t* button = lv_button_create(_root);

    lv_obj_set_size(button, width, 27);
    lv_obj_set_pos(button, VALUE_X, y);
    lv_obj_set_style_radius(button, 4, 0);

    return button;
}

void SettingsUI::update()
{
    updateDateTime();
}

void SettingsUI::updateDateTime()
{
    struct tm timeInfo;

    if (!getLocalTime(&timeInfo, 0))
    {
        lv_label_set_text(_dateLabel, "--/--/----");
        lv_label_set_text(_timeLabel, "--:-- --");
        return;
    }

    char buffer[20];

    strftime(
        buffer,
        sizeof(buffer),
        "%m/%d/%Y",
        &timeInfo
    );

    lv_label_set_text(_dateLabel, buffer);

    strftime(
        buffer,
        sizeof(buffer),
        "%I:%M %p",
        &timeInfo
    );

    const char* displayTime =
        buffer[0] == '0'
            ? buffer + 1
            : buffer;

    lv_label_set_text(_timeLabel, displayTime);
}

void SettingsUI::updateControls()
{
    if (_settings.ntpEnabled())
        lv_obj_add_state(_ntpSwitch, LV_STATE_CHECKED);
    else
        lv_obj_remove_state(_ntpSwitch, LV_STATE_CHECKED);

    lv_label_set_text(
        _unitsLabel,
        _settings.useMetric()
            ? "Metric (C)"
            : "Imperial (F)"
    );

    const String& timezone = _settings.timezone();

    if (timezone == "EST5EDT,M3.2.0,M11.1.0")
        lv_label_set_text(_timezoneLabel, "Eastern");
    else if (timezone == "CST6CDT,M3.2.0,M11.1.0")
        lv_label_set_text(_timezoneLabel, "Central");
    else if (timezone == "MST7MDT,M3.2.0,M11.1.0")
        lv_label_set_text(_timezoneLabel, "Mountain");
    else if (timezone == "PST8PDT,M3.2.0,M11.1.0")
        lv_label_set_text(_timezoneLabel, "Pacific");
    else
        lv_label_set_text(_timezoneLabel, "Custom");
}

void SettingsUI::ntpChanged(lv_event_t* event)
{
    auto* ui =
        static_cast<SettingsUI*>(
            lv_event_get_user_data(event)
        );

    bool enabled =
        lv_obj_has_state(
            ui->_ntpSwitch,
            LV_STATE_CHECKED
        );

    ui->_settings.setNtpEnabled(enabled);
}

void SettingsUI::unitsClicked(lv_event_t* event)
{
    auto* ui =
        static_cast<SettingsUI*>(
            lv_event_get_user_data(event)
        );

    ui->_settings.setUseMetric(
        !ui->_settings.useMetric()
    );

    ui->updateControls();
}

void SettingsUI::timezoneClicked(lv_event_t* event)
{
    auto* ui =
        static_cast<SettingsUI*>(
            lv_event_get_user_data(event)
        );

    const String& timezone = ui->_settings.timezone();

    if (timezone == "EST5EDT,M3.2.0,M11.1.0")
        ui->_settings.setTimezone("CST6CDT,M3.2.0,M11.1.0");
    else if (timezone == "CST6CDT,M3.2.0,M11.1.0")
        ui->_settings.setTimezone("MST7MDT,M3.2.0,M11.1.0");
    else if (timezone == "MST7MDT,M3.2.0,M11.1.0")
        ui->_settings.setTimezone("PST8PDT,M3.2.0,M11.1.0");
    else
        ui->_settings.setTimezone("EST5EDT,M3.2.0,M11.1.0");

    ui->_clock.timezoneChanged();

    ui->updateControls();
    ui->updateDateTime();
}