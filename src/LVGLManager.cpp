#include "LVGLManager.h"
#include <M5Unified.h>

static uint32_t lvglMillis()
{
    return static_cast<uint32_t>(millis());
}

uint16_t LVGLManager::_drawBuffer[
    LVGLManager::SCREEN_WIDTH * LVGLManager::BUFFER_LINES
];

void LVGLManager::begin()
{
    lv_init();

    // Let LVGL use Arduino's millisecond clock.
    lv_tick_set_cb(lvglMillis);

    lv_display_t* display =
        lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);

    lv_display_set_color_format(
        display,
        LV_COLOR_FORMAT_RGB565
    );

    lv_display_set_buffers(
        display,
        _drawBuffer,
        nullptr,
        sizeof(_drawBuffer),
        LV_DISPLAY_RENDER_MODE_PARTIAL
    );

    lv_display_set_flush_cb(
        display,
        flushDisplay
    );
}

void LVGLManager::update()
{
    lv_timer_handler();
}

void LVGLManager::flushDisplay(
    lv_display_t* display,
    const lv_area_t* area,
    uint8_t* pixelMap
)
{
    int32_t width =
        area->x2 - area->x1 + 1;

    int32_t height =
        area->y2 - area->y1 + 1;

    M5.Display.startWrite();

    M5.Display.setAddrWindow(
        area->x1,
        area->y1,
        width,
        height
    );

    M5.Display.writePixels(
        reinterpret_cast<uint16_t*>(pixelMap),
        width * height, 
        true
    );

    M5.Display.endWrite();

    lv_display_flush_ready(display);
}

void LVGLManager::printLvglMemory(const char *where)
{
    lv_mem_monitor_t m;
    lv_mem_monitor(&m);

    Serial.printf(
        "LVGL MEM %-20s total=%u free=%u biggest=%u used_cnt=%u free_cnt=%u max_used=%u used_pct=%u frag_pct=%u\n",
        where,
        (unsigned)m.total_size,
	(unsigned)m.free_size,
        (unsigned)m.free_biggest_size,
        (unsigned)m.used_cnt,
        (unsigned)m.free_cnt,
        (unsigned)m.max_used,
	(unsigned)m.used_pct,
	(unsigned)m.frag_pct
    );
}
