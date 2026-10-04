#include "SettingsUI.h"

SettingsUI::SettingsUI(lv_obj_t* parent)
{
    _root = lv_obj_create(parent);
    lv_obj_set_size(_root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(_root, lv_color_hex(0x101418), 0);
    lv_obj_set_style_bg_opa(_root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_root, 0, 0);
    lv_obj_set_style_pad_all(_root, 0, 0);
    lv_obj_set_scrollable(_root, false);

    lv_obj_t* label = lv_label_create(_root);
    lv_label_set_text(label, "Settings coming soon");
    lv_obj_set_style_text_color(label, lv_color_hex(0xF2F4F5), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_center(label);
}

SettingsUI::~SettingsUI()
{
    if (_root)
    {
        lv_obj_delete(_root);
        _root = nullptr;
    }
}

void SettingsUI::update()
{
}