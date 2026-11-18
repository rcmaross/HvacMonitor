#include <Arduino.h>
#include <Wire.h>
#include <M5Unified.h>
#include <Adafruit_ADS1X15.h>

// Nonblocking ADS1115 single-shot round-robin test.
// GPIO19 must connect to ALERT/RDY (external pull-up to 3.3 V).
constexpr int SDA_PIN = 32;
constexpr int SCL_PIN = 33;
constexpr int READY_PIN = 19;
constexpr uint8_t ADS_ADDRESS = 0x48;
constexpr uint32_t SLOT_MS = 333;
constexpr uint32_t TIMEOUT_MS = 30; // 475 SPS nominally takes ~2.1 ms
constexpr uint8_t CHANNELS[] = {0, 1, 3};
constexpr uint8_t NUM_CHANNELS = sizeof(CHANNELS);

Adafruit_ADS1115 ads;

volatile bool readyInterrupt = false;
bool connected = false;
bool pending = false;
uint8_t slot = 0;
uint8_t pendingChannel = 0;
uint32_t nextSlotMs = 0;
uint32_t conversionStartMs = 0;
uint32_t lastReportMs = 0;
uint32_t timeouts = 0;
uint32_t ioErrors = 0;
uint32_t samples[4] = {};
int16_t latest[4] = {};
bool valid[4] = {};

void IRAM_ATTR onReady() { readyInterrupt = true; }

bool connectADS() {
    if (!ads.begin(ADS_ADDRESS, &Wire)) return false;
    ads.setGain(GAIN_ONE);
    ads.setDataRate(RATE_ADS1115_475SPS);
    // startADCReading configures conversion-ready ALERT/RDY mode.
    return true;
}

bool startConversion(uint8_t channel) {
    readyInterrupt = false;
    pendingChannel = channel;
    ads.startADCReading(ADS1X15_REG_CONFIG_MUX_SINGLE_0 +
                        (static_cast<uint16_t>(channel) << 12), false);
    conversionStartMs = millis();
    pending = true;
    return true;
}

void report(uint32_t now) {
    Serial.printf("ms=%lu", static_cast<unsigned long>(now));
    for (uint8_t i = 0; i < NUM_CHANNELS; ++i) {
        const uint8_t ch = CHANNELS[i];
        if (valid[ch])
            Serial.printf("  A%u=%6d (%+.5f V) n=%lu", ch, latest[ch],
                          latest[ch] * 0.000125f,
                          static_cast<unsigned long>(samples[ch]));
        else
            Serial.printf("  A%u=-- n=0", ch);
    }
    Serial.printf("  timeouts=%lu ioErrors=%lu\n",
                  static_cast<unsigned long>(timeouts),
                  static_cast<unsigned long>(ioErrors));

    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setTextSize(2);
    M5.Display.setCursor(8, 8);
    M5.Display.println("ADS1115 async 475 SPS");
    for (uint8_t i = 0; i < NUM_CHANNELS; ++i) {
        const uint8_t ch = CHANNELS[i];
        M5.Display.setCursor(8, 48 + i * 42);
        if (valid[ch])
            M5.Display.printf("A%u %6d %+.3fV", ch, latest[ch], latest[ch] * 0.000125f);
        else
            M5.Display.printf("A%u waiting", ch);
    }
    M5.Display.setCursor(8, 190);
    M5.Display.printf("T/O:%lu I2C:%lu", static_cast<unsigned long>(timeouts),
                      static_cast<unsigned long>(ioErrors));
}

void setup() {
    Serial.begin(115200);
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setRotation(1);
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(100000);
    pinMode(READY_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(READY_PIN), onReady, FALLING);
    connected = connectADS();
    Serial.printf("ADS1115 async: SDA=%d SCL=%d ALERT=%d address=0x%02X connected=%d\n",
                  SDA_PIN, SCL_PIN, READY_PIN, ADS_ADDRESS, connected);
    nextSlotMs = millis();
    lastReportMs = millis();
}

void loop() {
    M5.update();
    const uint32_t now = millis();

    if (!connected) {
        static uint32_t lastRetry = 0;
        if (now - lastRetry >= 2000) {
            lastRetry = now;
            connected = connectADS();
            if (connected) nextSlotMs = now;
            else Serial.println("ADS1115 not found / configuration failed");
        }
        return;
    }

    if (pending) {
        if (readyInterrupt || digitalRead(READY_PIN) == LOW) {
            readyInterrupt = false;
            latest[pendingChannel] = ads.getLastConversionResults();
            valid[pendingChannel] = true;
            ++samples[pendingChannel];
            pending = false;
        } else if (now - conversionStartMs >= TIMEOUT_MS) {
            ++timeouts;
            pending = false;
            Serial.printf("ALERT timeout on A%u\n", pendingChannel);
        }
    }

    if (!pending && static_cast<int32_t>(now - nextSlotMs) >= 0) {
        if (!startConversion(CHANNELS[slot])) {
            ++ioErrors;
            connected = false;
            Serial.println("ADS1115 start conversion I2C error");
        }
        slot = (slot + 1) % NUM_CHANNELS;
        nextSlotMs += SLOT_MS;
        // Avoid a burst of catch-up conversions after a long interruption.
        if (static_cast<int32_t>(now - nextSlotMs) >= 0)
            nextSlotMs = now + SLOT_MS;
    }

    if (now - lastReportMs >= 1000) {
        lastReportMs = now;
        report(now);
    }
}
