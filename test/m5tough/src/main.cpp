#include <Arduino.h>
#include <M5Unified.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

// M5Tough (classic ESP32): DAC1 GPIO25, DAC2 GPIO26, PWM GPIO19.
// GPIO19 MUST have an RC low-pass filter before the target ADC input.
// Do not connect a 5V signal to any target input.
static constexpr uint8_t LARGE_PIN = 25;
static constexpr uint8_t SMALL_PIN = 26;
static constexpr uint8_t OUTSIDE_PIN = 19;
static constexpr uint8_t PWM_CHANNEL = 0;
static constexpr uint32_t BAUD = 115200;
static constexpr uint32_t STALE_MS = 30000;

// Match the production TempSensor.cpp 10k fixed / 10k B3950 NTC divider.
static constexpr float VCC = 3.3f;
static constexpr float R_FIXED = 10000.0f;
static constexpr float R_25 = 10000.0f;
static constexpr float BETA = 3950.0f;
static constexpr float T_25_K = 298.15f;

static char input[96];
static size_t used = 0;
static bool active = false;
static uint32_t lastUpdate = 0;
static float displayedF[3] = {NAN, NAN, NAN};

static void drawScreen() {
    auto& lcd = M5.Display;
    lcd.fillScreen(TFT_BLACK);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.setTextSize(2);
    lcd.setCursor(8, 10);
    lcd.println("HVAC Simulator");
    lcd.setTextSize(3);
    const char* names[3] = {"Large", "Small", "Outside"};
    for (int i = 0; i < 3; ++i) {
        lcd.setCursor(8, 55 + i * 48);
        if (isnan(displayedF[i])) lcd.printf("%s: --", names[i]);
        else lcd.printf("%s: %.1f F", names[i], displayedF[i]);
    }
    lcd.setTextSize(2);
    lcd.setCursor(8, 212);
    lcd.print(active ? "Receiving serial" : "Waiting for serial");
}

static float calibrateF(float requestedF, int channel) {
    static constexpr float commanded[] = {
        65.0f, 90.0f, 120.0f
    };

    static constexpr float measured[3][3] = {
        {72.1f, 96.7f, 128.7f},  // Large
        {72.8f, 98.6f, 130.6f},  // Small
        {70.9f, 96.7f, 130.6f}   // Outdoor
    };

    const float* readings = measured[channel];

    int segment = requestedF <= readings[1] ? 0 : 1;

    float fraction =
        (requestedF - readings[segment]) /
        (readings[segment + 1] - readings[segment]);

    return commanded[segment] +
           fraction * (commanded[segment + 1] -
                       commanded[segment]);
}

static float voltageForF(float f) {
    const float k = (f - 32.0f) * (5.0f / 9.0f) + 273.15f;
    const float r = R_25 * expf(BETA * (1.0f / k - 1.0f / T_25_K));
    return VCC * r / (R_FIXED + r);
}

static uint8_t to8bit(float voltage) {
    const float scaled = voltage * 255.0f / VCC;
    if (scaled <= 0.0f) return 0;
    if (scaled >= 255.0f) return 255;
    return static_cast<uint8_t>(scaled + 0.5f);
}

static void setZero() {
    dacWrite(LARGE_PIN, 0);
    dacWrite(SMALL_PIN, 0);
    ledcWrite(PWM_CHANNEL, 0);
}

static void process(char* line) {
    // Command: F,<large F>,<small F>,<outside F>
    if (strncmp(line, "F,", 2) != 0) {
        Serial.println("ERR expected F,large,small,outside");
        return;
    }
    float f[3];
    char* p = line + 2;
    for (int i = 0; i < 3; ++i) {
        char* end = nullptr;
        f[i] = strtof(p, &end);
        if (end == p || !isfinite(f[i]) || f[i] < -20.0f || f[i] > 200.0f) {
            Serial.println("ERR invalid temperature (-20..200 F)");
            return;
        }
        if (i < 2) {
            if (*end != ',') { Serial.println("ERR missing comma"); return; }
            p = end + 1;
        } else if (*end != '\0' && *end != '\r') {
            Serial.println("ERR trailing characters");
            return;
        }
    }

    uint8_t v[3] = {
        to8bit(voltageForF(calibrateF(f[0], 0))),
        to8bit(voltageForF(calibrateF(f[1], 1))),
        to8bit(voltageForF(calibrateF(f[2], 2)))
    };    
    dacWrite(LARGE_PIN, v[0]);
    dacWrite(SMALL_PIN, v[1]);
    ledcWrite(PWM_CHANNEL, v[2]);
    active = true;
    for (int i = 0; i < 3; ++i) displayedF[i] = f[i];
    lastUpdate = millis();
    drawScreen();
    Serial.printf("OK %.2f %.2f %.2f DAC=%u,%u PWM=%u\n",
                  f[0], f[1], f[2], v[0], v[1], v[2]);
}

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setRotation(1);
    Serial.begin(BAUD);
    // Wi-Fi is not started. PWM is 8-bit at 5 kHz; external RC filter
    // smooths the third channel. Core2/M5Tough target uses ESP32 Arduino 2.x.
    ledcSetup(PWM_CHANNEL, 5000, 8);
    ledcAttachPin(OUTSIDE_PIN, PWM_CHANNEL);
    setZero();
    drawScreen();
    Serial.println("READY M5Tough HVAC simulator: F,largeF,smallF,outsideF");
}

void loop() {
    while (Serial.available()) {
        const char c = static_cast<char>(Serial.read());
        if (c == '\n') {
            input[used] = '\0';
            process(input);
            used = 0;
        } else if (used < sizeof(input) - 1) {
            input[used++] = c;
        } else {
            used = 0;
            Serial.println("ERR line too long");
        }
    }
    if (active && millis() - lastUpdate > STALE_MS) {
        active = false;
        setZero();
        drawScreen();
        Serial.println("STALE outputs zero");
    }
}
