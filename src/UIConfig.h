#pragma once

namespace UIConfig
{
    constexpr int SCREEN_WIDTH  = 320;
    constexpr int SCREEN_HEIGHT = 240;

    constexpr int HEADER_HEIGHT = 40;
    constexpr int TAB_BAR_HEIGHT = 42;

    constexpr int CONTENT_Y = HEADER_HEIGHT;

    constexpr int CONTENT_HEIGHT =
        SCREEN_HEIGHT -
        HEADER_HEIGHT -
        TAB_BAR_HEIGHT;
}