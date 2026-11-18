#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <M5Unified.h>

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

// ADS1115 registers
constexpr uint8_t REG_CONVERSION = 0x00;
constexpr uint8_t REG_CONFIG = 0x01;
constexpr uint8_t REG_LO_THRESH = 0x02;
constexpr uint8_t REG_HI_THRESH = 0x03;

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

bool writeRegister(uint8_t reg, uint16_t value) {
    Wire.beginTransmission(ADS_ADDRESS);
    Wire.write(reg);
    Wire.write(static_cast<uint8_t>(value >> 8));
    Wire.write(static_cast<uint8_t>(value));
    return Wire.endTransmission() == 0;
}

bool readRegister(uint8_t reg, uint16_t& value) {
    Wire.beginTransmission(ADS_ADDRESS);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(ADS_ADDRESS, static_cast<uint8_t>(2)) != 2) return false;
    value = (static_cast<uint16_t>(Wire.read()) << 8) | Wire.read();
    return true;
}

bool connectADS() {
    Wire.beginTransmission(ADS_ADDRESS);
    if (Wire.endTransmission() != 0) return false;
    // Conversion-ready mode: high threshold MSB=1, low threshold MSB=0.
    // Comparator queue is enabled (00) in each conversion's CONFIG write.
    return writeRegister(REG_HI_THRESH, 0x8000) &&
           writeRegister(REG_LO_THRESH, 0x0000);
}

bool startConversion(uint8_t channel) {
    // OS=1, MUX=100+channel (single-ended), PGA=001 (+/-4.096V),
    // MODE=1 (single-shot), DR=110 (475 SPS), comparator traditional,
    // active low, nonlatching, assert after one conversion.
    const uint16_t config = 0x8000u |
                            (static_cast<uint16_t>(4 + channel) << 12) |
                            0x0200u | 0x0100u | 0x00C0u;
    readyInterrupt = false;
    pendingChannel = channel;
    if (!writeRegister(REG_CONFIG, config)) return false;
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
            uint16_t raw;
            if (readRegister(REG_CONVERSION, raw)) {
                latest[pendingChannel] = static_cast<int16_t>(raw);
                valid[pendingChannel] = true;
                ++samples[pendingChannel];
            } else {
                ++ioErrors;
            }
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
