#pragma once

#include <lvgl.h>
#include "ScreenUI.h"

class SettingsUI : public ScreenUI
{
public:
    SettingsUI(lv_obj_t* parent);
    virtual ~SettingsUI() override;

    void update() override;

private:
    lv_obj_t* _root = nullptr;
};