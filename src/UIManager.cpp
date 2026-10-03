#include "UIManager.h"

static constexpr int TAB_BAR_HEIGHT = 42;

UIManager::UIManager(Status& status)
    : _status(status)
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

    // Main content area
    _contentArea = lv_obj_create(screen);

    lv_obj_set_size(
        _contentArea,
        LV_PCT(100),
        240 - TAB_BAR_HEIGHT
    );

    lv_obj_align(
        _contentArea,
        LV_ALIGN_TOP_MID,
        0,
        0
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

void UIManager::createNavigation()
{
    lv_obj_t* screen = lv_screen_active();

    _tabBar = lv_obj_create(screen);

    lv_obj_set_size(
        _tabBar,
        LV_PCT(100),
        TAB_BAR_HEIGHT
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
    if (_statusUI)
    {
        delete _statusUI;
        _statusUI = nullptr;
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
            _statusUI =
                new StatusUI(_contentArea, _status);
            break;

        case Screen::Settings:
        {
            lv_obj_t* label =
                lv_label_create(_contentArea);

            lv_label_set_text(label, "Settings");
            lv_obj_center(label);
            break;
        }

        case Screen::Network:
        {
            lv_obj_t* label =
                lv_label_create(_contentArea);

            lv_label_set_text(label, "Network");
            lv_obj_center(label);
            break;
        }
    }
}

void UIManager::update()
{
    if (_statusUI)
        _statusUI->update();
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