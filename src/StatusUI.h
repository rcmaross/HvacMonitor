#pragma once

#include <lvgl.h>
#include "Status.h"
#include "ScreenUI.h"

class StatusUI : public ScreenUI
{
public:
    StatusUI(lv_obj_t* parent, Status& status);
    virtual ~StatusUI() override;

    void update() override;

private:
    struct TemperatureRow
    {
        lv_obj_t* name = nullptr;
        lv_obj_t* track = nullptr;
        lv_obj_t* marker = nullptr;
        lv_obj_t* value = nullptr;
    };

    Status& _status;

    lv_obj_t* _root = nullptr;

    TemperatureRow _large;
    TemperatureRow _small;
    TemperatureRow _outdoor;

    lv_obj_t* _scaleBar = nullptr;
    lv_obj_t* _scaleMin = nullptr;
    lv_obj_t* _scaleMax = nullptr;

    lv_obj_t* _systemStatus = nullptr;
    lv_obj_t* _performanceStatus = nullptr;

    float _displayMin = 0.0f;
    float _displayMax = 100.0f;

    void createTemperatureRow(
        lv_obj_t* parent,
        const char* name,
        int y,
        lv_color_t color,
        TemperatureRow& row
    );

    void createScale(lv_obj_t* parent);

    void updateTemperatureRow(
        TemperatureRow& row,
        float temperature
    );

    void updateScale(
        float large,
        float small,
        float outdoor
    );
};