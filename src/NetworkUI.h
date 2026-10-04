#pragma once

#include <lvgl.h>
#include "ScreenUI.h"

class NetworkUI : public ScreenUI
{
public:
    NetworkUI(lv_obj_t* parent);
    virtual ~NetworkUI() override;

    void update() override;

private:
    lv_obj_t* _root = nullptr;
};