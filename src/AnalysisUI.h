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
    lv_obj_t* _values[5] = {};
    lv_obj_t* _sliders[5] = {};
    lv_obj_t* _apply = nullptr;
    bool _dirty = false;
    static void changed(lv_event_t* event);
    static void apply(lv_event_t* event);
    void sync();
};
