#pragma once
#include "ScreenUI.h"
#include <lvgl.h>
class FirmwareUI : public ScreenUI {
public:
    explicit FirmwareUI(lv_obj_t* parent);
    ~FirmwareUI() override;
    void update() override;
private:
    lv_obj_t* _root = nullptr;
    lv_obj_t* _uptime = nullptr;
    uint32_t _lastUptimeSeconds = UINT32_MAX;
};
