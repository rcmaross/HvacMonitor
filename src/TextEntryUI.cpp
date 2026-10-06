#include "TextEntryUI.h"

TextEntryUI::TextEntryUI(const char* title,
                         const String& initialText,
                         bool passwordMode,
                         CompleteCallback completeCallback,
                         CancelCallback cancelCallback,
                         void* userData)
    : _completeCallback(completeCallback),
      _cancelCallback(cancelCallback),
      _userData(userData)
{
    (void)passwordMode; //to shut up the compiler for now

    //
    // Use the top layer so this covers the entire 320x240 display,
    // including the normal navigation bar.
    //

    _root = lv_obj_create(lv_layer_top());
    lv_obj_set_size(_root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(_root, 0, 0);
    lv_obj_set_style_bg_color(_root, lv_color_hex(0x101418), 0);
    lv_obj_set_style_bg_opa(_root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_root, 0, 0);
    lv_obj_set_style_pad_all(_root, 4, 0);
    lv_obj_set_scrollable(_root, false);

    //
    // Title
    //

    _titleLabel = lv_label_create(_root);
    lv_label_set_text(_titleLabel, title);
    lv_obj_set_pos(_titleLabel, 6, 5);
    lv_obj_set_style_text_color(_titleLabel, lv_color_hex(0xF2F4F5), 0);

    //
    // Cancel
    //

    _cancelButton = lv_button_create(_root);
    lv_obj_set_size(_cancelButton, 70, 30);
    lv_obj_align(_cancelButton, LV_ALIGN_TOP_RIGHT, -4, 0);

    lv_obj_t* cancelLabel = lv_label_create(_cancelButton);
    lv_label_set_text(cancelLabel, "Cancel");
    lv_obj_center(cancelLabel);

    lv_obj_add_event_cb(_cancelButton, cancelClicked, LV_EVENT_CLICKED, this);

    //
    // Text entry
    //

    _textArea = lv_textarea_create(_root);
    lv_obj_set_size(_textArea, 310, 36);
    lv_obj_set_pos(_textArea, 1, 38);

    lv_textarea_set_one_line(_textArea, true);
    lv_textarea_set_text(_textArea, initialText.c_str());

    //
    // We deliberately show the password while entering it.
    // On this small screen, being able to verify what was typed is
    // more useful than masking it.
    //

    lv_textarea_set_password_mode(_textArea, false);

    //
    // Keyboard
    //

    _keyboard = lv_keyboard_create(_root);
    lv_obj_set_size(_keyboard, 312, 158);
    lv_obj_align(_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_keyboard_set_textarea(_keyboard, _textArea);

    lv_obj_add_event_cb(_keyboard, keyboardReady, LV_EVENT_READY, this);
    lv_obj_add_event_cb(_keyboard, keyboardCancel, LV_EVENT_CANCEL, this);

    lv_obj_add_state(_textArea, LV_STATE_FOCUSED);
}

TextEntryUI::~TextEntryUI()
{
    if (_root)
    {
        lv_obj_delete(_root);
        _root = nullptr;
    }
}

void TextEntryUI::keyboardReady(lv_event_t* event)
{
    auto* ui = static_cast<TextEntryUI*>(lv_event_get_user_data(event));

    if (!ui)
        return;

    const char* value = lv_textarea_get_text(ui->_textArea);

    String text(value);

    if (ui->_completeCallback)
        ui->_completeCallback(text, ui->_userData);
}

void TextEntryUI::keyboardCancel(lv_event_t* event)
{
    auto* ui = static_cast<TextEntryUI*>(lv_event_get_user_data(event));

    if (!ui)
        return;

    if (ui->_cancelCallback)
        ui->_cancelCallback(ui->_userData);
}

void TextEntryUI::cancelClicked(lv_event_t* event)
{
    auto* ui = static_cast<TextEntryUI*>(lv_event_get_user_data(event));

    if (!ui)
        return;

    if (ui->_cancelCallback)
        ui->_cancelCallback(ui->_userData);
}