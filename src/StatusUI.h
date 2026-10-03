#pragma once

#include <lvgl.h>
#include "Status.h"

class StatusUI
{
public:
    StatusUI(lv_obj_t* parent, Status& status);
    ~StatusUI();

    void update();

private:
    struct Thermometer
    {
        lv_obj_t* tube = nullptr;
        lv_obj_t* fill = nullptr;
        lv_obj_t* bulb = nullptr;
        lv_obj_t* value = nullptr;
    };

    Status& _status;

    lv_obj_t* _root = nullptr;

    Thermometer _large;
    Thermometer _small;
    Thermometer _outdoor;

    lv_obj_t* _systemStatus = nullptr;
    lv_obj_t* _performanceStatus = nullptr;

    void createThermometer(
        lv_obj_t* parent,
        const char* name,
        int centerX,
        Thermometer& thermometer
    );

    void updateThermometer(
        Thermometer& thermometer,
        float temperature,
        lv_color_t color
    );
};