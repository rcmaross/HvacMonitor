#pragma once

class ScreenUI
{
public:
    virtual ~ScreenUI() = default;
    virtual void update() = 0;
};