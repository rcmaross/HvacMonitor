#include <unity.h>
#include <cmath>
#include <cstring>
#include "Status.h"
#include "../../src/TempSensor.cpp"
#include "../../src/Status.cpp"

static unsigned long fakeNow = 0;
static int adcValues[64] = {};
unsigned long millis() { return fakeNow; }
int analogRead(int pin) { return adcValues[pin]; }
void pinMode(int, int) {}

static void reset() {
    fakeNow = 0;
    for (int& value : adcValues) value = 2048;
}
// Set a sensor's filtered output without ADC noise or a 60-reading warm-up.
// These are the actual temperatures the production Status object reads.
static void setTemperature(TemperatureSensor& sensor, float c) {
    // The production class intentionally encapsulates its sample buffer.
    // Feed 60 identical ADC readings instead of changing production access rules.
    // A 10K/B3950 thermistor maps temperature to an ADC count.
    const float resistance = 10000.0f * expf(3950.0f * (1.0f / (c + 273.15f) - 1.0f / 298.15f));
    const int adc = (int)lroundf(4095.0f * resistance / (10000.0f + resistance));
    adcValues[sensor.pin()] = adc;
    for (int i = 0; i < 60; ++i) sensor.readCelsius();
}
static void sample(Status& status, TemperatureSensor& large, TemperatureSensor& small,
                   TemperatureSensor& outdoor, float l, float s, float a, unsigned long t) {
    setTemperature(large, l);
    setTemperature(small, s);
    setTemperature(outdoor, a);
    fakeNow = t;
    status.update();
    status.sampleCompressor();
}
void test_warmup_and_stable_off() {
    reset();
    TemperatureSensor l("large", 35), s("small", 36), a("outdoor", 34);
    Status status(&l, &s, &a);
    sample(status,l,s,a,20,20,15,0);
    TEST_ASSERT_EQUAL_INT((int)CompressorState::UNKNOWN,(int)status.compressorState());
    sample(status,l,s,a,20,20,15,10000);
    sample(status,l,s,a,20,20,15,20000);
    sample(status,l,s,a,20,20,15,30000);
    TEST_ASSERT_EQUAL_INT((int)CompressorState::OFF,(int)status.compressorState());
    TEST_ASSERT_FALSE(status.compressorHeating());
}
void test_fast_ramp_heating() {
    reset();
    TemperatureSensor l("large",35),s("small",36),a("ambient",34);
    Status status(&l,&s,&a);
    sample(status,l,s,a,20,20,10,0);
    sample(status,l,s,a,20,20,10,10000);
    sample(status,l,s,a,20,20,10,20000);
    sample(status,l,s,a,20,20,10,30000);
    sample(status,l,s,a,25,21,10,40000);
    TEST_ASSERT_EQUAL_INT((int)CompressorState::HEATING,(int)status.compressorState());
    TEST_ASSERT_TRUE(status.compressorHeating());
    TEST_ASSERT_TRUE(status.largeSlopeCPerMin() > 0.5f);
}
void test_plateau_and_shutdown() {
    reset();
    TemperatureSensor l("large",35),s("small",36),a("ambient",34);
    Status status(&l,&s,&a);
    sample(status,l,s,a,40,25,10,0);
    sample(status,l,s,a,40,25,10,10000);
    sample(status,l,s,a,40,25,10,20000);
    sample(status,l,s,a,40,25,10,30000);
    TEST_ASSERT_EQUAL_INT((int)CompressorState::PLATEAU,(int)status.compressorState());
    TEST_ASSERT_TRUE(status.compressorHeating());
    sample(status,l,s,a,25,20,10,40000);
    TEST_ASSERT_EQUAL_INT((int)CompressorState::OFF,(int)status.compressorState());
}
void test_standby_heating() {
    reset();
    TemperatureSensor l("large",35),s("small",36),a("ambient",34);
    Status status(&l,&s,&a);
    sample(status,l,s,a,25,20,10,0);
    sample(status,l,s,a,25,20,10,10000);
    sample(status,l,s,a,25,20,10,20000);
    sample(status,l,s,a,25,20,10,30000);
    sample(status,l,s,a,25.12f,20,10,40000); // ~0.24 C/min over 30 sec
    TEST_ASSERT_EQUAL_INT((int)CompressorState::STANDBY_HEATING,(int)status.compressorState());
}
void test_state_names() {
    TEST_ASSERT_EQUAL_STRING("OFF",Status::compressorStateName(CompressorState::OFF));
    TEST_ASSERT_EQUAL_STRING("HEATING",Status::compressorStateName(CompressorState::HEATING));
    TEST_ASSERT_EQUAL_STRING("STANDBY_HEATING",Status::compressorStateName(CompressorState::STANDBY_HEATING));
    TEST_ASSERT_EQUAL_STRING("PLATEAU",Status::compressorStateName(CompressorState::PLATEAU));
    TEST_ASSERT_EQUAL_STRING("DOWNSTAGING",Status::compressorStateName(CompressorState::DOWNSTAGING));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN",Status::compressorStateName(CompressorState::UNKNOWN));
}
int main(int,char**) {
    UNITY_BEGIN();
    RUN_TEST(test_warmup_and_stable_off);
    RUN_TEST(test_fast_ramp_heating);
    RUN_TEST(test_plateau_and_shutdown);
    RUN_TEST(test_standby_heating);
    RUN_TEST(test_state_names);
    return UNITY_END();
}
