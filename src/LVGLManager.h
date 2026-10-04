#pragma once

#include <lvgl.h>

class LVGLManager
{
public:
    void begin();
    void update();
    void printLvglMemory(const char *where);
private:
    static constexpr int SCREEN_WIDTH  = 320;
    static constexpr int SCREEN_HEIGHT = 240;

    // 20 scan lines = 12.8 KB at RGB565
    static constexpr int BUFFER_LINES = 20;

    static uint16_t _drawBuffer[SCREEN_WIDTH * BUFFER_LINES];

    static void flushDisplay( lv_display_t* display, const lv_area_t* area, uint8_t* pixelMap);
    static void readTouch(lv_indev_t* indev, lv_indev_data_t* data);
};