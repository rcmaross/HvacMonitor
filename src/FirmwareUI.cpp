#include "FirmwareUI.h"
#include "BuildInfo.h"
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
}
FirmwareUI::~FirmwareUI() { if (_root) lv_obj_delete(_root); }
