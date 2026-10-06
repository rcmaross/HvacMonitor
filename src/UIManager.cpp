#include "UIManager.h"
#include "UIConfig.h"

#include "StatusUI.h"
#include "SettingsUI.h"
#include "NetworkUI.h"

UIManager::UIManager(Status& status, Network& network, Settings& settings, Clock& clock)
    : _status(status), _network(network), _settings(settings), _clock(clock)
{
    lv_obj_t* screen = lv_screen_active();

    lv_obj_set_style_bg_color(
        screen,
        lv_color_hex(0x101418),
        0
    );

    lv_obj_set_style_bg_opa(
        screen,
        LV_OPA_COVER,
        0
    );

    createHeader();

    // Main content area
    _contentArea = lv_obj_create(screen);

    lv_obj_set_size(
        _contentArea,
        UIConfig::SCREEN_WIDTH,
        UIConfig::CONTENT_HEIGHT
    );

    lv_obj_set_pos(
        _contentArea,
        0,
        UIConfig::CONTENT_Y
    );

    lv_obj_set_style_bg_color(
        _contentArea,
        lv_color_hex(0x101418),
        0
    );

    lv_obj_set_style_bg_opa(
        _contentArea,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_style_border_width(_contentArea, 0, 0);
    lv_obj_set_style_pad_all(_contentArea, 0, 0);
    lv_obj_set_scrollable(_contentArea, false);

    createNavigation();

    showScreen(Screen::Status);
}

UIManager::~UIManager()
{
    clearCurrentScreen();
}

void UIManager::createHeader()
{
    lv_obj_t* screen = lv_screen_active();

    _header = lv_obj_create(screen);

    lv_obj_set_size(
        _header,
        UIConfig::SCREEN_WIDTH,
        UIConfig::HEADER_HEIGHT
    );

    lv_obj_set_pos(_header, 0, 0);

    lv_obj_set_style_bg_color(
        _header,
        lv_color_hex(0x181D22),
        0
    );

    lv_obj_set_style_bg_opa(
        _header,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_style_border_width(_header, 0, 0);
    lv_obj_set_style_pad_all(_header, 0, 0);
    lv_obj_set_scrollable(_header, false);

    //
    // Wi-Fi signal strength
    //

    for (int i = 0; i < 4; i++)
    {
        _wifiBars[i] = lv_obj_create(_header);

        lv_obj_set_size(
            _wifiBars[i],
            4,
            5 + i * 4
        );

        lv_obj_set_pos(
            _wifiBars[i],
            8 + i * 6,
            27 - i * 4
        );

        lv_obj_set_style_radius(_wifiBars[i], 1, 0);
        lv_obj_set_style_border_width(_wifiBars[i], 0, 0);
        lv_obj_set_style_pad_all(_wifiBars[i], 0, 0);
        lv_obj_set_style_bg_opa(_wifiBars[i], LV_OPA_COVER, 0);
    }

    //
    // Time
    //

    _timeLabel = lv_label_create(_header);
    lv_label_set_text(_timeLabel, "--:--");
    lv_obj_set_style_text_font(_timeLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(_timeLabel, lv_color_hex(0xF2F4F5), 0);
    lv_obj_align(_timeLabel, LV_ALIGN_CENTER, 0, 0);

    //
    // System state
    //
    // Gray = idle
    // Later: red = heating, blue = cooling
    //

    _stateDot = lv_obj_create(_header);
    lv_obj_set_size(_stateDot, 10, 10);
    lv_obj_set_style_radius(_stateDot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(_stateDot, 0, 0);
    lv_obj_set_style_pad_all(_stateDot, 0, 0);
    lv_obj_set_style_bg_color(_stateDot, lv_color_hex(0x808080), 0);
    lv_obj_set_style_bg_opa(_stateDot, LV_OPA_COVER, 0);
    lv_obj_align(_stateDot, LV_ALIGN_RIGHT_MID, -54, 0);

    //
    // Efficiency
    //
    // Hard-coded until the performance algorithm exists.
    //

    _efficiencyLabel = lv_label_create(_header);
    lv_label_set_text(_efficiencyLabel, "100%");
    lv_obj_set_style_text_font(_efficiencyLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(_efficiencyLabel, lv_color_hex(0x00C853), 0);
    lv_obj_align(_efficiencyLabel, LV_ALIGN_RIGHT_MID, -8, 0);
}
void UIManager::updateHeader()
{
    int bars = 0;

    if (_network.isConnected())
    {
        int32_t rssi = _network.rssi();

        if (rssi >= -55)
            bars = 4;
        else if (rssi >= -65)
            bars = 3;
        else if (rssi >= -75)
            bars = 2;
        else
            bars = 1;
    }

    for (int i = 0; i < 4; i++)
    {
        lv_color_t color =
            i < bars
                ? lv_color_hex(0xF2F4F5)
                : lv_color_hex(0x40474D);

        lv_obj_set_style_bg_color(_wifiBars[i], color, 0);
    }

    //
    // Time will remain --:-- until system time is established.
    //

    struct tm timeInfo;

    if (getLocalTime(&timeInfo, 0))
    {
        char timeBuffer[16];

        strftime(
            timeBuffer,
            sizeof(timeBuffer),
            "%I:%M %p",
            &timeInfo
        );

        //
        // Remove the leading zero from 12-hour time.
        //

        const char* displayTime =
            timeBuffer[0] == '0'
                ? timeBuffer + 1
                : timeBuffer;

        lv_label_set_text(_timeLabel, displayTime);
    }
    else
    {
        lv_label_set_text(_timeLabel, "--:--");
    }
}

void UIManager::createNavigation()
{
    lv_obj_t* screen = lv_screen_active();

    _tabBar = lv_obj_create(screen);

    lv_obj_set_size(
        _tabBar,
        UIConfig::SCREEN_WIDTH,
        UIConfig::TAB_BAR_HEIGHT
    );

    lv_obj_align(
        _tabBar,
        LV_ALIGN_BOTTOM_MID,
        0,
        0
    );

    lv_obj_set_style_bg_color(
        _tabBar,
        lv_color_hex(0x181D22),
        0
    );

    lv_obj_set_style_bg_opa(
        _tabBar,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_style_border_width(_tabBar, 0, 0);
    lv_obj_set_style_pad_all(_tabBar, 0, 0);
    lv_obj_set_scrollable(_tabBar, false);

    // Use flex to divide the three buttons evenly.
    lv_obj_set_flex_flow(
        _tabBar,
        LV_FLEX_FLOW_ROW
    );

    _statusButton = lv_button_create(_tabBar);
    _settingsButton = lv_button_create(_tabBar);
    _networkButton = lv_button_create(_tabBar);

    lv_obj_set_flex_grow(_statusButton, 1);
    lv_obj_set_flex_grow(_settingsButton, 1);
    lv_obj_set_flex_grow(_networkButton, 1);

    lv_obj_set_height(_statusButton, LV_PCT(100));
    lv_obj_set_height(_settingsButton, LV_PCT(100));
    lv_obj_set_height(_networkButton, LV_PCT(100));

    lv_obj_set_style_radius(_statusButton, 0, 0);
    lv_obj_set_style_radius(_settingsButton, 0, 0);
    lv_obj_set_style_radius(_networkButton, 0, 0);

    lv_obj_t* label;

    label = lv_label_create(_statusButton);
    lv_label_set_text(label, "STATUS");
    lv_obj_center(label);

    label = lv_label_create(_settingsButton);
    lv_label_set_text(label, "SETTINGS");
    lv_obj_center(label);

    label = lv_label_create(_networkButton);
    lv_label_set_text(label, "NETWORK");
    lv_obj_center(label);

    lv_obj_add_event_cb(
        _statusButton,
        statusButtonClicked,
        LV_EVENT_CLICKED,
        this
    );

    lv_obj_add_event_cb(
        _settingsButton,
        settingsButtonClicked,
        LV_EVENT_CLICKED,
        this
    );

    lv_obj_add_event_cb(
        _networkButton,
        networkButtonClicked,
        LV_EVENT_CLICKED,
        this
    );
}

void UIManager::clearCurrentScreen()
{
    if (_currentUI)
    {
        delete _currentUI;
        _currentUI = nullptr;
    }

    // SettingsUI and NetworkUI will eventually be handled here too.
}

void UIManager::showScreen(Screen screen)
{
    clearCurrentScreen();

    _currentScreen = screen;

    switch (screen)
    {
        case Screen::Status:
            _currentUI = new StatusUI(_contentArea, _status);
            break;

        case Screen::Settings:
            _currentUI = new SettingsUI(_contentArea, _settings, _clock);
            break;

        case Screen::Network:
            _currentUI = new NetworkUI(_contentArea, _network);
            break;
    }

    if (_currentUI) {
        lv_obj_update_layout(_contentArea);
        _currentUI->update();
    }

}
void UIManager::update()
{
    updateHeader();

    if (_currentUI)
        _currentUI->update();
}

void UIManager::statusButtonClicked(lv_event_t* event)
{
    auto* ui =
        static_cast<UIManager*>(
            lv_event_get_user_data(event)
        );

    ui->showScreen(Screen::Status);
}

void UIManager::settingsButtonClicked(lv_event_t* event)
{
    auto* ui =
        static_cast<UIManager*>(
            lv_event_get_user_data(event)
        );

    ui->showScreen(Screen::Settings);
}

void UIManager::networkButtonClicked(lv_event_t* event)
{
    auto* ui =
        static_cast<UIManager*>(
            lv_event_get_user_data(event)
        );

    ui->showScreen(Screen::Network);
}