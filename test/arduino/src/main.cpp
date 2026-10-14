#include <Arduino.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "DiagDeck.h"

// PWM pins deliberately avoid the diagnostic daughter-card pins.
// IMPORTANT: Each pin needs the resistor divider + capacitor in README.md.
static const uint8_t PWM_PINS[3] = {8, 9, 10};
static const float SUPPLY_V = 3.3f;
static const float PWM_HIGH_V = 5.0f;
static const float SERIES_OHM = 10000.0f;
static const float SHUNT_OHM = 15000.0f;
static const float OUTPUT_MAX_V = PWM_HIGH_V * SHUNT_OHM / (SERIES_OHM + SHUNT_OHM);
static const float BETA = 3950.0f;
static const float R25 = 10000.0f;
static const float FIXED_R = 10000.0f;
static const float K25 = 298.15f;
static const unsigned long STALE_MS = 30000UL;

DiagDeck deck;
float tempsF[3] = {NAN, NAN, NAN};
bool paused = false;
bool haveData = false;
unsigned long lastRowMs = 0;
char line[80];
uint8_t lineLength = 0;
uint8_t page = 0;

float voltageForF(float fahrenheit) {
    float kelvin = (fahrenheit - 32.0f) * (5.0f / 9.0f) + 273.15f;
    float resistance = R25 * expf(BETA * (1.0f / kelvin - 1.0f / K25));
    return SUPPLY_V * resistance / (FIXED_R + resistance);
}

void setOutputs() {
    for (uint8_t i = 0; i < 3; ++i) {
        if (!haveData || isnan(tempsF[i])) {
            analogWrite(PWM_PINS[i], 0);
            continue;
        }
        float volts = voltageForF(tempsF[i]);
        float duty = volts / OUTPUT_MAX_V * 255.0f;
        if (duty < 0.0f) duty = 0.0f;
        if (duty > 255.0f) duty = 255.0f;
        analogWrite(PWM_PINS[i], (uint8_t)(duty + 0.5f));
    }
}

void display() {
    char row[17];
    if (!haveData) {
        deck.printLine(0, "Awaiting CSV...");
        deck.printLine(1, "USB 115200 baud");
        return;
    }
    if (page == 0) {
        snprintf(row, sizeof(row), "Large %5dF", (int)roundf(tempsF[0]));
        deck.printLine(0, row);
        snprintf(row, sizeof(row), "Small %5dF", (int)roundf(tempsF[1]));
        deck.printLine(1, row);
    } else {
        snprintf(row, sizeof(row), "Outside %4dF", (int)roundf(tempsF[2]));
        deck.printLine(0, row);
        deck.printLine(1, paused ? "PAUSED" : "REPLAY RUNNING");
    }
}

void onA() {
    paused = !paused;
    deck.setLedColor(2, paused ? LED_BLUE : LED_GREEN);
    Serial.println(paused ? "PAUSED" : "RUNNING");
    display();
}

void onB() {
    page = (page + 1) % 2;
    display();
}

void parseLine(char* input) {
    // Protocol: F,<largeF>,<smallF>,<outsideF>\n
    if (strncmp(input, "F,", 2) != 0) {
        Serial.println("ERR expected F,large,small,outside");
        return;
    }
    float parsed[3];
    char* ptr = input + 2;
    for (uint8_t i = 0; i < 3; ++i) {
        char* end = nullptr;
        parsed[i] = strtod(ptr, &end);
        if (end == ptr || !isfinite(parsed[i]) ||
            parsed[i] < -20.0f || parsed[i] > 200.0f) {
            Serial.println("ERR invalid temperature");
            return;
        }
        if (i < 2) {
            if (*end != ',') { Serial.println("ERR missing comma"); return; }
            ptr = end + 1;
        } else if (*end != '\0' && *end != '\r') {
            Serial.println("ERR trailing characters");
            return;
        }
    }
    if (paused) { Serial.println("PAUSED"); return; }
    for (uint8_t i = 0; i < 3; ++i) tempsF[i] = parsed[i];
    haveData = true;
    lastRowMs = millis();
    setOutputs();
    deck.setLedColor(1, LED_GREEN);
    display();
    Serial.println("OK");
}

void setup() {
    Serial.begin(115200);
    for (uint8_t i = 0; i < 3; ++i) {
        pinMode(PWM_PINS[i], OUTPUT);
        analogWrite(PWM_PINS[i], 0);
    }
    deck.begin();
    deck.registerButtonACallback(onA);
    deck.registerButtonBCallback(onB);
    deck.setLedColor(1, LED_RED);
    deck.setLedColor(2, LED_GREEN);
    display();
    Serial.println("Mega HVAC simulator ready: F,largeF,smallF,outsideF");
}

void loop() {
    deck.update();
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\n') {
            line[lineLength] = '\0';
            parseLine(line);
            lineLength = 0;
        } else if (lineLength < sizeof(line) - 1) {
            line[lineLength++] = c;
        } else {
            lineLength = 0;
            Serial.println("ERR line too long");
        }
    }
    if (haveData && millis() - lastRowMs > STALE_MS) {
        haveData = false;
        setOutputs();
        deck.setLedColor(1, LED_RED);
        display();
        Serial.println("STALE: outputs set to zero");
    }
}
