#include "DiagDeck.h"
#include <LiquidCrystal.h>

// Fixed wiring of the diagnostic daughter card.
const uint8_t LCD_RS = 2;
const uint8_t LCD_E  = 3;
const uint8_t LCD_D4 = 4;
const uint8_t LCD_D5 = 5;
const uint8_t LCD_D6 = 6;
const uint8_t LCD_D7 = 7;

const uint8_t RED_1 = 44; const uint8_t GREEN_1 = 42; const uint8_t BLUE_1 = 40;
const uint8_t RED_2 = 38; const uint8_t GREEN_2 = 36; const uint8_t BLUE_2 = 34;
const uint8_t BUTTON_A = 46; const uint8_t BUTTON_B = 48;

static LiquidCrystal _lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

DiagDeck::DiagDeck() {
    _btnACallback = nullptr;
    _btnBCallback = nullptr;
    _lastBtnAState = HIGH;
    _lastBtnBState = HIGH;
}

void DiagDeck::begin() {
    pinMode(13, OUTPUT);
    digitalWrite(13, LOW);
    pinMode(BUTTON_A, INPUT_PULLUP);
    pinMode(BUTTON_B, INPUT_PULLUP);
    pinMode(RED_1, OUTPUT); pinMode(GREEN_1, OUTPUT); pinMode(BLUE_1, OUTPUT);
    pinMode(RED_2, OUTPUT); pinMode(GREEN_2, OUTPUT); pinMode(BLUE_2, OUTPUT);
    delay(100);
    _lcd.begin(16, 2);
    _lcd.clear();
}

void DiagDeck::update() { checkButtons(); }

void DiagDeck::checkButtons() {
    bool currA = digitalRead(BUTTON_A);
    bool currB = digitalRead(BUTTON_B);
    if (currA == LOW && _lastBtnAState == HIGH) {
        if (_btnACallback != nullptr) _btnACallback();
        delay(150);
    }
    _lastBtnAState = currA;
    if (currB == LOW && _lastBtnBState == HIGH) {
        if (_btnBCallback != nullptr) _btnBCallback();
        delay(150);
    }
    _lastBtnBState = currB;
}

void DiagDeck::setLedColor(uint8_t ledNumber, LedColor color) {
    if (ledNumber == 1) {
        digitalWrite(RED_1, (color == LED_RED) ? HIGH : LOW);
        digitalWrite(GREEN_1, (color == LED_GREEN) ? HIGH : LOW);
        digitalWrite(BLUE_1, (color == LED_BLUE) ? HIGH : LOW);
    } else if (ledNumber == 2) {
        digitalWrite(RED_2, (color == LED_RED) ? HIGH : LOW);
        digitalWrite(GREEN_2, (color == LED_GREEN) ? HIGH : LOW);
        digitalWrite(BLUE_2, (color == LED_BLUE) ? HIGH : LOW);
    }
}

void DiagDeck::printLine(uint8_t row, const char* text) {
    if (row > 1) return;
    _lcd.setCursor(0, row);
    char buffer[17];
    snprintf(buffer, sizeof(buffer), "%-16s", text);
    _lcd.print(buffer);
}
void DiagDeck::clearDisplay() { _lcd.clear(); }
void DiagDeck::registerButtonACallback(void (*callback)()) { _btnACallback = callback; }
void DiagDeck::registerButtonBCallback(void (*callback)()) { _btnBCallback = callback; }
