#include "StatusUI.h"

#include <math.h>
#include <stdio.h>

namespace
{
    constexpr int TUBE_WIDTH = 12;
    constexpr int TUBE_HEIGHT = 58;
    constexpr int BULB_SIZE = 24;

    constexpr int TUBE_TOP = 32;

    constexpr float TEMP_MIN = 0.0f;
    constexpr float TEMP_MAX = 100.0f;

    const lv_color_t BACKGROUND =
        lv_color_hex(0x101418);

    const lv_color_t PRIMARY_TEXT =
        lv_color_hex(0xF2F4F5);

    const lv_color_t SECONDARY_TEXT =
        lv_color_hex(0xAEB7BF);

    const lv_color_t THERMOMETER_BORDER =
        lv_color_hex(0x78838C);

    const lv_color_t LARGE_COLOR =
        lv_color_hex(0x4CAF50);

    const lv_color_t SMALL_COLOR =
        lv_color_hex(0xF44336);

    const lv_color_t OUTDOOR_COLOR =
        lv_color_hex(0x2196F3);
}

StatusUI::StatusUI(
    lv_obj_t* parent,
    Status& status
)
    : _status(status)
{
    _root = lv_obj_create(parent);

    lv_obj_set_size(
        _root,
        LV_PCT(100),
        LV_PCT(100)
    );

    lv_obj_set_style_bg_color(
        _root,
        BACKGROUND,
        0
    );

    lv_obj_set_style_bg_opa(
        _root,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_style_border_width(
        _root,
        0,
        0
    );

    lv_obj_set_style_pad_all(
        _root,
        0,
        0
    );

    lv_obj_set_scrollable(
        _root,
        false
    );

    //
    // Three temperature channels
    //

    createThermometer(
        _root,
        "LARGE",
        53,
        _large
    );

    createThermometer(
        _root,
        "SMALL",
        160,
        _small
    );

    createThermometer(
        _root,
        "OUTDOOR",
        267,
        _outdoor
    );

    //
    // HVAC state
    //

    _systemStatus =
        lv_label_create(_root);

    lv_label_set_text(
        _systemStatus,
        "System not running"
    );

    lv_obj_set_style_text_color(
        _systemStatus,
        PRIMARY_TEXT,
        0
    );

    lv_obj_set_style_text_font(
        _systemStatus,
        &lv_font_montserrat_14,
        0
    );

    lv_obj_align(
        _systemStatus,
        LV_ALIGN_TOP_MID,
        0,
        151
    );

    //
    // Performance state
    //

    _performanceStatus =
        lv_label_create(_root);

    lv_label_set_text(
        _performanceStatus,
        "Performance status unavailable"
    );

    lv_obj_set_style_text_color(
        _performanceStatus,
        SECONDARY_TEXT,
        0
    );

    lv_obj_set_style_text_font(
        _performanceStatus,
        &lv_font_montserrat_14,
        0
    );

    lv_obj_align(
        _performanceStatus,
        LV_ALIGN_TOP_MID,
        0,
        173
    );
}

StatusUI::~StatusUI()
{
    if (_root)
    {
        lv_obj_delete(_root);
        _root = nullptr;
    }
}

void StatusUI::createThermometer(
    lv_obj_t* parent,
    const char* name,
    int centerX,
    Thermometer& thermometer
)
{
    //
    // Channel name
    //

    lv_obj_t* nameLabel =
        lv_label_create(parent);

    lv_label_set_text(
        nameLabel,
        name
    );

    lv_obj_set_style_text_color(
        nameLabel,
        SECONDARY_TEXT,
        0
    );

    lv_obj_set_style_text_font(
        nameLabel,
        &lv_font_montserrat_14,
        0
    );

    constexpr int LABEL_WIDTH = 100;

    lv_obj_set_width(
        nameLabel,
        LABEL_WIDTH
    );

    lv_obj_set_style_text_align(
        nameLabel,
        LV_TEXT_ALIGN_CENTER,
        0
    );

    lv_obj_set_pos(
        nameLabel,
        centerX - LABEL_WIDTH / 2,
        8
    );

    //
    // Thermometer tube
    //

    thermometer.tube =
        lv_obj_create(parent);

    lv_obj_set_size(
        thermometer.tube,
        TUBE_WIDTH,
        TUBE_HEIGHT
    );

    lv_obj_set_pos(
        thermometer.tube,
        centerX - TUBE_WIDTH / 2,
        TUBE_TOP
    );

    lv_obj_set_style_bg_opa(
        thermometer.tube,
        LV_OPA_TRANSP,
        0
    );

    lv_obj_set_style_border_width(
        thermometer.tube,
        2,
        0
    );

    lv_obj_set_style_border_color(
        thermometer.tube,
        THERMOMETER_BORDER,
        0
    );

    lv_obj_set_style_radius(
        thermometer.tube,
        TUBE_WIDTH / 2,
        0
    );

    lv_obj_set_style_pad_all(
        thermometer.tube,
        2,
        0
    );

    lv_obj_set_scrollable(
        thermometer.tube,
        false
    );

    //
    // Liquid inside tube
    //

    thermometer.fill =
        lv_obj_create(thermometer.tube);

    lv_obj_set_width(
        thermometer.fill,
        4
    );

    lv_obj_set_height(
        thermometer.fill,
        1
    );

    lv_obj_align(
        thermometer.fill,
        LV_ALIGN_BOTTOM_MID,
        0,
        0
    );

    lv_obj_set_style_border_width(
        thermometer.fill,
        0,
        0
    );

    lv_obj_set_style_radius(
        thermometer.fill,
        2,
        0
    );

    lv_obj_set_style_pad_all(
        thermometer.fill,
        0,
        0
    );

    lv_obj_set_scrollable(
        thermometer.fill,
        false
    );

    //
    // Bulb
    //

    thermometer.bulb =
        lv_obj_create(parent);

    lv_obj_set_size(
        thermometer.bulb,
        BULB_SIZE,
        BULB_SIZE
    );

    lv_obj_set_pos(
        thermometer.bulb,
        centerX - BULB_SIZE / 2,
        TUBE_TOP + TUBE_HEIGHT - 5
    );

    lv_obj_set_style_radius(
        thermometer.bulb,
        LV_RADIUS_CIRCLE,
        0
    );

    lv_obj_set_style_border_width(
        thermometer.bulb,
        2,
        0
    );

    lv_obj_set_style_border_color(
        thermometer.bulb,
        THERMOMETER_BORDER,
        0
    );

    lv_obj_set_style_pad_all(
        thermometer.bulb,
        0,
        0
    );

    lv_obj_set_scrollable(
        thermometer.bulb,
        false
    );

    //
    // Numeric temperature
    //

    thermometer.value =
        lv_label_create(parent);

    lv_label_set_text(
        thermometer.value,
        "--.- F"
    );

    lv_obj_set_style_text_color(
        thermometer.value,
        PRIMARY_TEXT,
        0
    );

    lv_obj_set_style_text_font(
        thermometer.value,
        &lv_font_montserrat_20,
        0
    );

    lv_obj_align(
        thermometer.value,
        LV_ALIGN_TOP_LEFT,
        centerX,
        113
    );

    lv_obj_set_x(
        thermometer.value,
        centerX -
            lv_obj_get_width(thermometer.value) / 2
    );
}

void StatusUI::updateThermometer(
    Thermometer& thermometer,
    float temperature,
    lv_color_t color
)
{
    if (isnan(temperature))
    {
        lv_label_set_text(
            thermometer.value,
            "--.- F"
        );

        lv_obj_set_height(
            thermometer.fill,
            1
        );

        return;
    }

    //
    // Numeric value
    //

    char buffer[16];

    snprintf(
        buffer,
        sizeof(buffer),
        "%.1f F",
        temperature
    );

    lv_label_set_text(
        thermometer.value,
        buffer
    );

    //
    // Keep value centered after its width changes.
    //

    lv_obj_t* parent =
        lv_obj_get_parent(thermometer.value);

    int centerX =
        lv_obj_get_x(thermometer.tube) +
        TUBE_WIDTH / 2;

    lv_obj_set_x(
        thermometer.value,
        centerX -
            lv_obj_get_width(thermometer.value) / 2
    );

    //
    // Clamp graphical temperature.
    //

    float displayTemp = temperature;

    if (displayTemp < TEMP_MIN)
        displayTemp = TEMP_MIN;

    if (displayTemp > TEMP_MAX)
        displayTemp = TEMP_MAX;

    float fraction =
        (displayTemp - TEMP_MIN) /
        (TEMP_MAX - TEMP_MIN);

    constexpr int MAX_FILL_HEIGHT =
        TUBE_HEIGHT - 8;

    int fillHeight =
        1 +
        static_cast<int>(
            fraction *
            (MAX_FILL_HEIGHT - 1)
        );

    lv_obj_set_height(
        thermometer.fill,
        fillHeight
    );

    //
    // Apply channel color.
    //

    lv_obj_set_style_bg_color(
        thermometer.fill,
        color,
        0
    );

    lv_obj_set_style_bg_opa(
        thermometer.fill,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_style_bg_color(
        thermometer.bulb,
        color,
        0
    );

    lv_obj_set_style_bg_opa(
        thermometer.bulb,
        LV_OPA_COVER,
        0
    );
}

void StatusUI::update()
{
    updateThermometer(
        _large,
        _status.largePipeTemperature(),
        LARGE_COLOR
    );

    updateThermometer(
        _small,
        _status.smallPipeTemperature(),
        SMALL_COLOR
    );

    updateThermometer(
        _outdoor,
        _status.outdoorTemperature(),
        OUTDOOR_COLOR
    );

    //
    // Placeholder until Status provides HVAC state.
    //

    lv_label_set_text(
        _systemStatus,
        "System not running"
    );

    lv_label_set_text(
        _performanceStatus,
        "Performance status unavailable"
    );
}