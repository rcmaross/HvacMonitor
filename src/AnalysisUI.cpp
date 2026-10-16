#include "AnalysisUI.h"
#include <stdio.h>
static lv_obj_t* label(lv_obj_t* parent, const char* text) {
    lv_obj_t* l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_color(l, lv_color_hex(0xf2f4f5), 0);
    return l;
}
static lv_obj_t* button(lv_obj_t* parent, const char* text, lv_event_cb_t cb, void* user) {
    lv_obj_t* b = lv_button_create(parent);
    lv_obj_set_size(b, 110, 36);
    lv_obj_t* l = label(b, text);
    lv_obj_center(l);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, user);
    return b;
}
AnalysisUI::AnalysisUI(lv_obj_t* parent, Analysis& analysis) : _analysis(analysis) {
    _root = lv_obj_create(parent);
    lv_obj_set_size(_root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(_root, lv_color_hex(0x101418), 0);
    lv_obj_set_style_border_width(_root, 0, 0);
    lv_obj_set_style_pad_all(_root, 8, 0);
    lv_obj_set_flex_flow(_root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(_root, 8, 0);
    lv_obj_set_scroll_dir(_root, LV_DIR_VER);
    label(_root, "LAST 24 HOURS");
    _summary = label(_root, "Loading History...");
    _levels = label(_root, "");
    label(_root, "Auto Calibrate");
    _auto = lv_switch_create(_root);
    lv_obj_add_event_cb(_auto, changed, LV_EVENT_VALUE_CHANGED, this);
    label(_root, "Low / Medium");
    _lowValue = label(_root, "");
    _low = lv_slider_create(_root);
    lv_obj_set_width(_low, LV_PCT(90));
    lv_slider_set_range(_low, 0, 49);
    lv_obj_add_event_cb(_low, changed, LV_EVENT_VALUE_CHANGED, this);
    label(_root, "Medium / High");
    _highValue = label(_root, "");
    _high = lv_slider_create(_root);
    lv_obj_set_width(_high, LV_PCT(90));
    lv_slider_set_range(_high, 1, 50);
    lv_obj_add_event_cb(_high, changed, LV_EVENT_VALUE_CHANGED, this);
    _ignoreValue = label(_root, "");
    button(_root, "OFF -10s", decrement, this);
    button(_root, "OFF +10s", increment, this);
    _apply = button(_root, "Recalculate", apply, this);
    _ignore = _analysis.ignoreOff();
    lv_slider_set_value(_low, _analysis.low(), LV_ANIM_OFF);
    lv_slider_set_value(_high, _analysis.high(), LV_ANIM_OFF);
    if (_analysis.autoMode()) lv_obj_add_state(_auto, LV_STATE_CHECKED);
    update();
}
AnalysisUI::~AnalysisUI() { if (_root) lv_obj_delete(_root); }
void AnalysisUI::changed(lv_event_t* event) {
    auto* ui = static_cast<AnalysisUI*>(lv_event_get_user_data(event));
    ui->_dirty = true;
    ui->update();
}
void AnalysisUI::decrement(lv_event_t* event) {
    auto* ui = static_cast<AnalysisUI*>(lv_event_get_user_data(event));
    ui->_ignore = ui->_ignore >= 10 ? ui->_ignore - 10 : 0;
    ui->_dirty = true; ui->update();
}
void AnalysisUI::increment(lv_event_t* event) {
    auto* ui = static_cast<AnalysisUI*>(lv_event_get_user_data(event));
    if (ui->_ignore < 120) ui->_ignore += 10;
    ui->_dirty = true; ui->update();
}
void AnalysisUI::apply(lv_event_t* event) {
    auto* ui = static_cast<AnalysisUI*>(lv_event_get_user_data(event));
    bool autoMode = lv_obj_has_state(ui->_auto, LV_STATE_CHECKED);
    ui->_analysis.setAuto(autoMode);
    if (!autoMode) ui->_analysis.setThresholds(lv_slider_get_value(ui->_low), lv_slider_get_value(ui->_high));
    ui->_analysis.setIgnoreOff(ui->_ignore);
    ui->_analysis.recalculate();
    ui->_dirty = false;
    ui->update();
}
void AnalysisUI::update() {
    if (_analysis.loading()) {
        lv_label_set_text(_summary, "Loading History...");
    } else {
        char buf[96];
        String data = _analysis.json();
        // Read percentages directly from the shared analysis model JSON.
        auto value = [&data](const char* key) {
            String token = String((char)34) + key + String((char)34) + ":";
            int i = data.indexOf(token);
            return i < 0 ? 0.0f : data.substring(i + token.length()).toFloat();
        };
        snprintf(buf, sizeof(buf), "OFF  %.1f%%       ON  %.1f%%", value("off"), value("on"));
        lv_label_set_text(_summary, buf);
        snprintf(buf, sizeof(buf), "LOW %.1f%%  MED %.1f%%  HIGH %.1f%%",
                 value("lowPercent"), value("mediumPercent"), value("highPercent"));
        lv_label_set_text(_levels, buf);
    }
    bool automatic = lv_obj_has_state(_auto, LV_STATE_CHECKED);
    if (automatic) {
        lv_obj_add_state(_low, LV_STATE_DISABLED);
        lv_obj_add_state(_high, LV_STATE_DISABLED);
    } else {
        lv_obj_remove_state(_low, LV_STATE_DISABLED);
        lv_obj_remove_state(_high, LV_STATE_DISABLED);
    }
    if (!_dirty) {
        if (_analysis.autoMode()) lv_obj_add_state(_auto, LV_STATE_CHECKED);
        else lv_obj_remove_state(_auto, LV_STATE_CHECKED);
        lv_slider_set_value(_low, _analysis.low(), LV_ANIM_OFF);
        lv_slider_set_value(_high, _analysis.high(), LV_ANIM_OFF);
        _ignore = _analysis.ignoreOff();
    }
    char buf[48];
    snprintf(buf, sizeof(buf), "%ld C", (long)lv_slider_get_value(_low));
    lv_label_set_text(_lowValue, buf);
    snprintf(buf, sizeof(buf), "%ld C", (long)lv_slider_get_value(_high));
    lv_label_set_text(_highValue, buf);
    snprintf(buf, sizeof(buf), "Ignore OFF <= %d seconds", _ignore);
    lv_label_set_text(_ignoreValue, buf);
}
