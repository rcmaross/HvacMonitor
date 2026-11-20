#include "AnalysisUI.h"
#include <stdio.h>
static lv_obj_t* makeLabel(lv_obj_t* parent, const char* text) {
    lv_obj_t* l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_color(l, lv_color_hex(0xf2f4f5), 0);
    return l;
}
AnalysisUI::AnalysisUI(lv_obj_t* parent, Analysis& analysis) : _analysis(analysis) {
    _root = lv_obj_create(parent);
    lv_obj_set_size(_root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(_root, lv_color_hex(0x101418), 0);
    lv_obj_set_style_border_width(_root, 0, 0);
    lv_obj_set_style_pad_all(_root, 8, 0);
    lv_obj_set_flex_flow(_root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(_root, 7, 0);
    lv_obj_set_scroll_dir(_root, LV_DIR_VER);
    makeLabel(_root, "HEATING ANALYSIS - LAST 24H");
    _summary = makeLabel(_root, "Loading history...");
    makeLabel(_root, "ELECTRICAL ANALYSIS SETTINGS");
    const char* names[] = {
        "OFF threshold (A)", "Max Power (W)", "Nominal voltage (V)",
        "Startup exclusion (s)", "Minimum Delta T (C)"
    };
    const int mins[] = {0, 100, 100, 0, 1};
    const int maxs[] = {200, 50000, 300, 900, 500};
    for (int i = 0; i < 5; ++i) {
        makeLabel(_root, names[i]);
        _values[i] = makeLabel(_root, "");
        _sliders[i] = lv_slider_create(_root);
        lv_obj_set_width(_sliders[i], LV_PCT(95));
        lv_slider_set_range(_sliders[i], mins[i], maxs[i]);
        lv_obj_add_event_cb(_sliders[i], changed, LV_EVENT_VALUE_CHANGED, this);
    }
    _apply = lv_button_create(_root);
    lv_obj_set_size(_apply, 185, 40);
    makeLabel(_apply, "Apply / Rebuild");
    lv_obj_add_event_cb(_apply, apply, LV_EVENT_CLICKED, this);
    makeLabel(_root, "Load = 240V x current / max power");
    makeLabel(_root, "Efficiency A/C: lower is better");
    sync();
    update();
}
AnalysisUI::~AnalysisUI() { if (_root) lv_obj_delete(_root); }
void AnalysisUI::sync() {
    const LoadConfig& c = _analysis.loadConfig();
    const int values[] = {
        (int)(c.offThresholdA * 100 + .5f),
        (int)(c.referenceW + .5f),
        (int)(c.voltageV + .5f),
        (int)c.startupSeconds,
        (int)(c.minDeltaC * 10 + .5f)
    };
    for (int i = 0; i < 5; ++i)
        lv_slider_set_value(_sliders[i], values[i], LV_ANIM_OFF);
}
void AnalysisUI::changed(lv_event_t* event) {
    auto* ui = static_cast<AnalysisUI*>(lv_event_get_user_data(event));
    ui->_dirty = true;
    ui->update();
}
void AnalysisUI::apply(lv_event_t* event) {
    auto* ui = static_cast<AnalysisUI*>(lv_event_get_user_data(event));
    const float threshold = lv_slider_get_value(ui->_sliders[0]) / 100.0f;
    const float watts = lv_slider_get_value(ui->_sliders[1]);
    const float voltage = lv_slider_get_value(ui->_sliders[2]);
    const uint32_t startup = lv_slider_get_value(ui->_sliders[3]);
    const float delta = lv_slider_get_value(ui->_sliders[4]) / 10.0f;
    if (ui->_analysis.configureLoad(threshold, watts, voltage, startup, delta)) {
        ui->_dirty = false;
        ui->sync();
    }
    ui->update();
}
void AnalysisUI::update() {
    char buf[160];
    if (_analysis.loading()) {
        lv_label_set_text(_summary, "Loading history...");
    } else {
        const LoadSummary t = _analysis.loadSummary();
        snprintf(buf, sizeof(buf),
            "Runtime: %.1f min\nFull-load: %.1f min\nAverage load: %.1f%%",
            t.runtimeMinutes, t.fullLoadMinutes, t.avgLoadPct);
        lv_label_set_text(_summary, buf);
    }
    const int v[] = {
        lv_slider_get_value(_sliders[0]), lv_slider_get_value(_sliders[1]),
        lv_slider_get_value(_sliders[2]), lv_slider_get_value(_sliders[3]),
        lv_slider_get_value(_sliders[4])
    };
    snprintf(buf, sizeof(buf), "%.2f A", v[0]/100.0f); lv_label_set_text(_values[0], buf);
    snprintf(buf, sizeof(buf), "%d W", v[1]); lv_label_set_text(_values[1], buf);
    snprintf(buf, sizeof(buf), "%d V", v[2]); lv_label_set_text(_values[2], buf);
    snprintf(buf, sizeof(buf), "%d seconds", v[3]); lv_label_set_text(_values[3], buf);
    snprintf(buf, sizeof(buf), "%.1f C", v[4]/10.0f); lv_label_set_text(_values[4], buf);
    if (!_dirty) sync();
}
