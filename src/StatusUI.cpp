#include "StatusUI.h"
#include "App.h"
#include <math.h>
#include <stdio.h>
namespace
{
    //
    // Layout
    //
    constexpr int LEFT_MARGIN = 6;
    constexpr int RIGHT_MARGIN = 6;
    constexpr int NAME_WIDTH = 78;
    constexpr int VALUE_WIDTH = 40;
    constexpr int COLUMN_GAP = 4;
    constexpr int NAME_X = LEFT_MARGIN;
    constexpr int GRAPH_X = NAME_X + NAME_WIDTH + COLUMN_GAP;
    constexpr int ROW_SPACING = 22;
    constexpr int ROW_LARGE_Y   = 4;
    constexpr int ROW_SMALL_Y   = ROW_LARGE_Y + ROW_SPACING;
    constexpr int ROW_OUTDOOR_Y = ROW_SMALL_Y + ROW_SPACING;
    constexpr int SCALE_Y = ROW_OUTDOOR_Y + ROW_SPACING;
    constexpr int SYSTEM_STATUS_Y = SCALE_Y + ROW_SPACING;
    constexpr int PERFORMANCE_STATUS_Y =  SYSTEM_STATUS_Y + ROW_SPACING;
    constexpr int TRACK_Y_OFFSET = 8;
    constexpr int TRACK_HEIGHT = 3;
    constexpr int MARKER_SIZE = 10;

    //
    // Scale endpoints now sit beside the gradient rather than
    // underneath it.
    //

    constexpr int SCALE_MIN_WIDTH = 29;
    constexpr int SCALE_GAP = 3;
    constexpr int SCALE_MAX_WIDTH = 29;
    constexpr int SCALE_HEIGHT = 7;

    //
    // Dynamic scale
    //

    constexpr float MIN_DISPLAY_SPAN = 20.0f;
    constexpr float SCALE_PADDING = 5.0f;
    constexpr float SCALE_INCREMENT = 5.0f;

    //
    // Colors
    //

    const lv_color_t BACKGROUND = lv_color_hex(0x101418);
    const lv_color_t PRIMARY_TEXT = lv_color_hex(0xF2F4F5);
    const lv_color_t SECONDARY_TEXT = lv_color_hex(0xAEB7BF);
    const lv_color_t TRACK_COLOR = lv_color_hex(0x505860);
    const lv_color_t LARGE_COLOR = lv_color_hex(0x4CAF50);
    const lv_color_t SMALL_COLOR = lv_color_hex(0xF44336);
    const lv_color_t OUTDOOR_COLOR = lv_color_hex(0x2196F3);

    //
    // Spectrum colors
    //

    const lv_color_t SPECTRUM_COLD = lv_color_hex(0x1546A0);
    const lv_color_t SPECTRUM_HOT = lv_color_hex(0xD62828);

    float roundDown(float value, float increment)
    {
        return floorf(value / increment) * increment;
    }

    float roundUp(float value, float increment)
    {
        return ceilf(value / increment) * increment;
    }

    int valueX(lv_obj_t* parent)
    {
        return lv_obj_get_content_width(parent) - RIGHT_MARGIN - VALUE_WIDTH;
    }

    int graphWidth(lv_obj_t* parent)
    {
        return valueX(parent) - COLUMN_GAP - GRAPH_X;
    }
}

StatusUI::StatusUI(lv_obj_t* parent, Status& status) : _status(status)
{
    _root = lv_obj_create(parent);
    lv_obj_set_size(_root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(_root, BACKGROUND, 0);
    lv_obj_set_style_bg_opa(_root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_root, 0, 0);
    lv_obj_set_style_pad_all(_root, 0, 0);
    lv_obj_set_scrollable(_root, false);

    // Resolve the percentage-based root size before any child
    // geometry is calculated from its content width.
    lv_obj_update_layout(_root);

    //
    // Temperature rows
    //

    createTemperatureRow(_root, "LARGE", ROW_LARGE_Y, LARGE_COLOR, _large);
    createTemperatureRow(_root, "SMALL", ROW_SMALL_Y, SMALL_COLOR, _small);
    createTemperatureRow(_root, "OUTDOOR", ROW_OUTDOOR_Y, OUTDOOR_COLOR, _outdoor);

    //
    // Shared temperature spectrum
    //

    createScale(_root);

    //
    // HVAC state
    //

    _systemStatus = lv_label_create(_root);
    lv_label_set_text(_systemStatus, "System not running");
    lv_obj_set_style_text_color(_systemStatus, PRIMARY_TEXT, 0);
    lv_obj_set_style_text_font(_systemStatus, &lv_font_montserrat_14, 0);
    lv_obj_align(_systemStatus, LV_ALIGN_TOP_MID, 0, SYSTEM_STATUS_Y);

    //
    // Performance state
    //

    _performanceStatus = lv_label_create(_root);
    lv_label_set_text(_performanceStatus, "Performance status unavailable");
    lv_obj_set_style_text_color(_performanceStatus, SECONDARY_TEXT, 0);
    lv_obj_set_style_text_font(_performanceStatus, &lv_font_montserrat_14, 0);
    lv_obj_align(_performanceStatus, LV_ALIGN_TOP_MID, 0, PERFORMANCE_STATUS_Y);
}

StatusUI::~StatusUI()
{
    if (_root)
    {
        lv_obj_delete(_root);
        _root = nullptr;
    }
}

void StatusUI::createTemperatureRow(lv_obj_t* parent, const char* name, int y, lv_color_t color, TemperatureRow& row)
{
    //
    // Sensor name
    //

    row.name = lv_label_create(parent);
    lv_label_set_text(row.name, name);
    lv_obj_set_width(row.name, NAME_WIDTH);
    lv_obj_set_pos(row.name, NAME_X, y);
    lv_obj_set_style_text_color(row.name, color, 0);
    lv_obj_set_style_text_font(row.name, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(row.name, LV_TEXT_ALIGN_LEFT, 0);

    //
    // Horizontal track
    //

    row.track = lv_obj_create(parent);
    lv_obj_set_size(row.track, graphWidth(parent), TRACK_HEIGHT);
    lv_obj_set_pos(row.track, GRAPH_X, y + TRACK_Y_OFFSET);
    lv_obj_set_style_bg_color(row.track, TRACK_COLOR, 0);
    lv_obj_set_style_bg_opa(row.track, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(row.track, 0, 0);
    lv_obj_set_style_radius(row.track, TRACK_HEIGHT / 2, 0);
    lv_obj_set_style_pad_all(row.track, 0, 0);
    lv_obj_set_scrollable(row.track, false);

    //
    // Temperature marker
    //

    row.marker = lv_obj_create(parent);
    lv_obj_set_size(row.marker, MARKER_SIZE, MARKER_SIZE);
    lv_obj_set_style_radius(row.marker, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(row.marker, color, 0);
    lv_obj_set_style_bg_opa(row.marker, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(row.marker, 1, 0);
    lv_obj_set_style_border_color(row.marker, PRIMARY_TEXT, 0);
    lv_obj_set_style_pad_all(row.marker, 0, 0);
    lv_obj_set_scrollable(row.marker, false);

    //
    // Numeric temperature
    //

    row.value = lv_label_create(parent);
    lv_label_set_text(row.value, "--.-");
    lv_obj_set_width(row.value, VALUE_WIDTH);
    lv_obj_align(row.value, LV_ALIGN_TOP_RIGHT, -RIGHT_MARGIN, y);
    lv_obj_set_style_text_color(row.value, PRIMARY_TEXT, 0);

    //
    // Smaller than the previous 20-point temperature display.
    //

    lv_obj_set_style_text_font(row.value, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(row.value, LV_TEXT_ALIGN_RIGHT, 0);
}

void StatusUI::createScale(lv_obj_t* parent)
{
    int scaleX = GRAPH_X;
    int scaleWidth = graphWidth(parent);
    int scaleMinX = scaleX - SCALE_GAP - SCALE_MIN_WIDTH;
    int scaleMaxX = scaleX + scaleWidth + SCALE_GAP;

    //
    // Minimum value - immediately to the left of the spectrum.
    //

    _scaleMin = lv_label_create(parent);
    lv_label_set_text(_scaleMin, "--");
    lv_obj_set_width(_scaleMin, SCALE_MIN_WIDTH);
    lv_obj_set_pos(_scaleMin, scaleMinX, SCALE_Y - 5);
    lv_obj_set_style_text_color(_scaleMin, SECONDARY_TEXT, 0);
    lv_obj_set_style_text_font(_scaleMin, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(_scaleMin, LV_TEXT_ALIGN_RIGHT, 0);

    //
    // Spectrum
    //

    _scaleBar = lv_obj_create(parent);
    lv_obj_set_size(_scaleBar, scaleWidth, SCALE_HEIGHT);
    lv_obj_set_pos(_scaleBar, scaleX, SCALE_Y);
    lv_obj_set_style_border_width(_scaleBar, 0, 0);
    lv_obj_set_style_radius(_scaleBar, SCALE_HEIGHT / 2, 0);
    lv_obj_set_style_pad_all(_scaleBar, 0, 0);
    lv_obj_set_scrollable(_scaleBar, false);
    lv_obj_set_style_bg_color(_scaleBar, SPECTRUM_COLD, 0);
    lv_obj_set_style_bg_grad_color(_scaleBar, SPECTRUM_HOT, 0);
    lv_obj_set_style_bg_grad_dir(_scaleBar, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_bg_opa(_scaleBar, LV_OPA_COVER, 0);

    //
    // Maximum value - immediately to the right of the spectrum.
    //

    _scaleMax = lv_label_create(parent);
    lv_label_set_text(_scaleMax, "--");
    lv_obj_set_width(_scaleMax, SCALE_MAX_WIDTH);
    lv_obj_set_pos(_scaleMax, scaleMaxX, SCALE_Y - 5);
    lv_obj_set_style_text_color(_scaleMax, SECONDARY_TEXT, 0);
    lv_obj_set_style_text_font(_scaleMax, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(_scaleMax, LV_TEXT_ALIGN_LEFT, 0);
}

void StatusUI::updateScale(float large, float small, float outdoor)
{
    //
    // Find the minimum and maximum valid readings.
    //

    bool haveTemperature = false;
    float lowTemp = 0.0f;
    float highTemp = 0.0f;

    const float temperatures[] =
    {
        large,
        small,
        outdoor
    };

    for (float temperature : temperatures)
    {
        if (isnan(temperature)) continue;

        if (!haveTemperature)
        {
            lowTemp = temperature;
            highTemp = temperature;
            haveTemperature = true;
        }
        else
        {
            if (temperature < lowTemp) lowTemp = temperature;
            if (temperature > highTemp) highTemp = temperature;
        }
    }

    if (!haveTemperature)
    {
        _displayMin = 0.0f;
        _displayMax = 100.0f;
        lv_label_set_text(_scaleMin, "--");
        lv_label_set_text(_scaleMax, "--");
        return;
    }

    //
    // Determine the span required to contain all sensors
    // plus padding.
    //

    float requiredSpan = (highTemp - lowTemp) + (2.0f * SCALE_PADDING);
    float displaySpan = requiredSpan;

    if (displaySpan < MIN_DISPLAY_SPAN) displaySpan = MIN_DISPLAY_SPAN;

    //
    // Center the range around the current readings.
    //

    float center = (lowTemp + highTemp) / 2.0f;
    float rawMin = center - displaySpan / 2.0f;
    float rawMax = center + displaySpan / 2.0f;

    //
    // Round outward to clean 5-degree boundaries.
    //

    _displayMin = roundDown(rawMin, SCALE_INCREMENT);
    _displayMax = roundUp(rawMax, SCALE_INCREMENT);

    //
    // Update endpoint labels.
    //

    char buffer[16];

    snprintf(buffer, sizeof(buffer), "%.0f", _displayMin);
    lv_label_set_text(_scaleMin, buffer);

    snprintf(buffer, sizeof(buffer), "%.0f", _displayMax);
    lv_label_set_text(_scaleMax, buffer);
}

void StatusUI::updateTemperatureRow(TemperatureRow& row, float temperature)
{
    if (isnan(temperature))
    {
        lv_label_set_text(row.value, "--.-");

        //
        // LVGL 9.6 dedicated setter.
        //

        lv_obj_set_hidden(row.marker, true);
        return;
    }

    lv_obj_set_hidden(row.marker, false);

    //
    // Numeric temperature
    //

    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%.1f", temperature);
    lv_label_set_text(row.value, buffer);

    //
    // Map the temperature to the shared graph.
    //

    float fraction = (temperature - _displayMin) / (_displayMax - _displayMin);

    if (fraction < 0.0f) fraction = 0.0f;
    if (fraction > 1.0f) fraction = 1.0f;

    int trackX = lv_obj_get_x(row.track);
    int trackWidth = lv_obj_get_width(row.track);
    int markerCenterX = trackX + static_cast<int>(fraction * trackWidth);
    int trackY = lv_obj_get_y(row.track);

    lv_obj_set_pos(row.marker, markerCenterX - MARKER_SIZE / 2, trackY - (MARKER_SIZE - TRACK_HEIGHT) / 2);
}

void StatusUI::update()
{
    float large = app.convertTempFromCelsius(_status.largePipeTemperature());
    float small = app.convertTempFromCelsius(_status.smallPipeTemperature());
    float outdoor = app.convertTempFromCelsius(_status.outdoorTemperature());

    //
    // Calculate the common scale before positioning markers.
    //

    updateScale(large, small, outdoor);

    updateTemperatureRow(_large, large);
    updateTemperatureRow(_small, small);
    updateTemperatureRow(_outdoor, outdoor);

    //
    // Placeholder until Status provides HVAC state.
    //

    lv_label_set_text(_systemStatus, "System not running");
    lv_label_set_text(_performanceStatus, "Performance status unavailable");
}