#include <Arduino.h>
#include <Wire.h>
#include <M5Unified.h>
#include <Adafruit_ADS1X15.h>
#include <math.h>

// M5Tough external Port A: SDA=GPIO32, SCL=GPIO33.
// Change these if the ADS1115 is wired to a different I2C bus.
constexpr int SDA_PIN = 32;
constexpr int SCL_PIN = 33;
constexpr uint8_t ADS_ADDRESS = 0x48; // ADDR connected to GND
constexpr uint32_t SAMPLE_INTERVAL_MS = 1000;

Adafruit_ADS1115 ads;
bool adsFound = false;
uint32_t lastSample = 0;
uint32_t lastRetry = 0;

void drawMessage(const char* line1, const char* line2) {
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setTextSize(2);
    M5.Display.setCursor(8, 12);
    M5.Display.println("ADS1115 test");
    M5.Display.setCursor(8, 60);
    M5.Display.println(line1);
    M5.Display.setCursor(8, 94);
    M5.Display.println(line2);
}

bool connectADS() {
    if (!ads.begin(ADS_ADDRESS, &Wire)) {
        Serial.printf("ERROR: ADS1115 not found at 0x%02X on SDA=%d SCL=%d\n",
                      ADS_ADDRESS, SDA_PIN, SCL_PIN);
        drawMessage("ADS1115 missing", "Check I2C wiring");
        return false;
    }
    // +/-4.096V full-scale (125 uV/LSB). Inputs must stay within
    // GND..VDD regardless of the selected programmable gain.
    ads.setGain(GAIN_ONE);
    ads.setDataRate(RATE_ADS1115_128SPS);
    Serial.printf("ADS1115 detected at 0x%02X, gain +/-4.096V, 128 SPS\n", ADS_ADDRESS);
    drawMessage("ADS1115 detected", "Sampling A0..A3");
    return true;
}

void setup() {
    Serial.begin(115200);
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setRotation(1);
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(100000);
    delay(250);
    Serial.println("\nM5Tough ADS1115 hardware test");
    Serial.println("A0..A3 single-ended; readings in counts and volts");
    adsFound = connectADS();
}

void loop() {
    M5.update();
    const uint32_t now = millis();
    if (!adsFound) {
        if (now - lastRetry >= 2000) {
            lastRetry = now;
            adsFound = connectADS();
        }
        delay(10);
        return;
    }
    if (now - lastSample < SAMPLE_INTERVAL_MS) return;
    lastSample = now;

    int16_t raw[4];
    float volts[4];
    for (int ch = 0; ch < 4; ++ch) {
        raw[ch] = ads.readADC_SingleEnded(ch);
        volts[ch] = ads.computeVolts(raw[ch]);
    }

    Serial.printf("ms=%lu", static_cast<unsigned long>(now));
    for (int ch = 0; ch < 4; ++ch) {
        Serial.printf("  A%d=%6d (%+.5f V)", ch, raw[ch], volts[ch]);
    }
    Serial.println();

    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setTextSize(2);
    M5.Display.setCursor(8, 8);
    M5.Display.println("ADS1115 / 0x48");
    for (int ch = 0; ch < 4; ++ch) {
        M5.Display.setCursor(8, 48 + ch * 42);
        M5.Display.printf("A%d %6d  %+.3fV", ch, raw[ch], volts[ch]);
    }
}
