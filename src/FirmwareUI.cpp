#include "FirmwareUI.h"
#include "BuildInfo.h"
#include <Arduino.h>
#include <stdio.h>
FirmwareUI::FirmwareUI(lv_obj_t* parent) {
    _root = lv_obj_create(parent);
    lv_obj_set_size(_root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(_root, lv_color_hex(0x101418), 0);
    lv_obj_set_style_border_width(_root, 0, 0);
    lv_obj_set_style_pad_all(_root, 8, 0);
    lv_obj_set_flex_flow(_root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(_root, LV_DIR_VER);
    const char* values[][2] = {
        {"Version", BuildInfo::version()},
        {"Commit", BuildInfo::commit()},
        {"Build", BuildInfo::buildType()},
        {"Built", BuildInfo::buildDate()}
    };
    for (const auto& pair : values) {
        lv_obj_t* label = lv_label_create(_root);
        char line[128];
        snprintf(line, sizeof(line), "%s: %s", pair[0], pair[1]);
        lv_label_set_text(label, line);
        lv_obj_set_width(label, LV_PCT(100));
        lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_color(label, lv_color_hex(0xF2F4F5), 0);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    }
    _uptime = lv_label_create(_root);
    lv_obj_set_style_text_color(_uptime, lv_color_hex(0xF2F4F5), 0);
    lv_obj_set_style_text_font(_uptime, &lv_font_montserrat_14, 0);
    update();
}
void FirmwareUI::update() {
    const uint32_t seconds = millis() / 1000;
    if (!_uptime || seconds == _lastUptimeSeconds) return;
    _lastUptimeSeconds = seconds;
    char line[96];
    snprintf(line, sizeof(line), "Uptime: %lu d %02lu:%02lu:%02lu",
        (unsigned long)(seconds / 86400), (unsigned long)((seconds / 3600) % 24),
        (unsigned long)((seconds / 60) % 60), (unsigned long)(seconds % 60));
    lv_label_set_text(_uptime, line);
}
FirmwareUI::~FirmwareUI() { if (_root) lv_obj_delete(_root); }
