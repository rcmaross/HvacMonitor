#pragma once
#include <lvgl.h>
#include "ScreenUI.h"
#include "Analysis.h"
class AnalysisUI : public ScreenUI {
public:
    AnalysisUI(lv_obj_t* parent, Analysis& analysis);
    ~AnalysisUI() override;
    void update() override;
private:
    Analysis& _analysis;
    lv_obj_t* _root = nullptr;
    lv_obj_t* _summary = nullptr;
    lv_obj_t* _levels = nullptr;
    lv_obj_t* _auto = nullptr;
    lv_obj_t* _low = nullptr;
    lv_obj_t* _high = nullptr;
    lv_obj_t* _lowValue = nullptr;
    lv_obj_t* _highValue = nullptr;
    lv_obj_t* _ignoreValue = nullptr;
    lv_obj_t* _apply = nullptr;
    lv_obj_t* _detectLowPower = nullptr;
    int _ignore = 10;
    bool _dirty = false;
    static void changed(lv_event_t* event);
    static void decrement(lv_event_t* event);
    static void increment(lv_event_t* event);
    static void apply(lv_event_t* event);
};
