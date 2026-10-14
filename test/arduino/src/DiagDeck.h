#ifndef DIAG_DECK_H
#define DIAG_DECK_H

#include <Arduino.h>

enum LedColor { LED_OFF = 0, LED_RED = 1, LED_GREEN = 2, LED_BLUE = 3 };

class DiagDeck {
public:
    DiagDeck();
    void begin();
    void update();
    void setLedColor(uint8_t ledNumber, LedColor color);
    void printLine(uint8_t row, const char* text);
    void clearDisplay();
    void registerButtonACallback(void (*callback)());
    void registerButtonBCallback(void (*callback)());
private:
    bool _lastBtnAState;
    bool _lastBtnBState;
    void (*_btnACallback)();
    void (*_btnBCallback)();
    void checkButtons();
};
#endif
