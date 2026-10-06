#pragma once

#include <lvgl.h>
#include <Arduino.h>

class TextEntryUI
{
public:
    using CompleteCallback = void (*)(const String& text, void* userData);
    using CancelCallback = void (*)(void* userData);

    TextEntryUI(const char* title,
                const String& initialText,
                bool passwordMode,
                CompleteCallback completeCallback,
                CancelCallback cancelCallback,
                void* userData);

    ~TextEntryUI();

private:
    lv_obj_t* _root = nullptr;
    lv_obj_t* _titleLabel = nullptr;
    lv_obj_t* _textArea = nullptr;
    lv_obj_t* _keyboard = nullptr;
    lv_obj_t* _cancelButton = nullptr;

    CompleteCallback _completeCallback = nullptr;
    CancelCallback _cancelCallback = nullptr;
    void* _userData = nullptr;

    static void keyboardReady(lv_event_t* event);
    static void keyboardCancel(lv_event_t* event);
    static void cancelClicked(lv_event_t* event);
};