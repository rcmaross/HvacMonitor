#include <unity.h>
#include <cmath>
#include "TempSensor.h"
#include "../../src/TempSensor.cpp"

unsigned long millis() { return 0; }
static constexpr float LSB = 0.000125f;
static int16_t rawForCelsius(float c) {
    const float r = 10000.0f * expf(3950.0f * (1.0f / (c + 273.15f) - 1.0f / 298.15f));
    return (int16_t)lroundf((3.3f * r / (10000.0f + r)) / LSB);
}
static float expectedCelsius(int16_t raw) {
    const float v = raw * LSB;
    const float r = 10000.0f * v / (3.3f - v);
    return 1.0f / (logf(r / 10000.0f) / 3950.0f + 1.0f / 298.15f) - 273.15f;
}
void test_initial_missing_sample_is_nan() {
    Ads1115Sampler sampler;
    TemperatureSensor sensor("large", sampler, 0);
    TEST_ASSERT_TRUE(std::isnan(sensor.readCelsius()));
}
void test_constant_sample_and_metadata() {
    Ads1115Sampler sampler;
    TemperatureSensor sensor("large", sampler, 0);
    TEST_ASSERT_EQUAL_STRING("large", sensor.name());
    TEST_ASSERT_EQUAL_INT(0, sensor.pin());
    const int16_t raw = rawForCelsius(25.0f);
    for (int i = 0; i < 30; ++i) {
        sampler.inject(0, raw);
        TEST_ASSERT_FLOAT_WITHIN(0.005f, expectedCelsius(raw), sensor.readCelsius());
    }
}
void test_sequence_deduplication() {
    Ads1115Sampler sampler;
    TemperatureSensor sensor("large", sampler, 0);
    sampler.inject(0, rawForCelsius(20));
    const float first = sensor.readCelsius();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, first, sensor.readCelsius());
    sampler.inject(0, rawForCelsius(30));
    TEST_ASSERT_TRUE(sensor.readCelsius() > first);
}
void test_rolling_average_replaces_old_samples() {
    Ads1115Sampler sampler;
    TemperatureSensor sensor("large", sampler, 0);
    for (int i = 0; i < TEMP_AVERAGE_SAMPLES; ++i) {
        sampler.inject(0, rawForCelsius(20));
        sensor.readCelsius();
    }
    for (int i = 0; i < TEMP_AVERAGE_SAMPLES; ++i) {
        sampler.inject(0, rawForCelsius(30));
        sensor.readCelsius();
    }
    TEST_ASSERT_FLOAT_WITHIN(0.01f, expectedCelsius(rawForCelsius(30)), sensor.readCelsius());
}
void test_invalid_voltage_does_not_replace_average() {
    Ads1115Sampler sampler;
    TemperatureSensor sensor("large", sampler, 0);
    sampler.inject(0, rawForCelsius(25));
    const float expected = sensor.readCelsius();
    sampler.inject(0, 0);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, expected, sensor.readCelsius());
    sampler.inject(0, 30000); // Above 3.3 V.
    TEST_ASSERT_FLOAT_WITHIN(0.001f, expected, sensor.readCelsius());
}
void test_channels_are_independent() {
    Ads1115Sampler sampler;
    TemperatureSensor a("large", sampler, 0), b("small", sampler, 1);
    sampler.inject(0, rawForCelsius(20));
    sampler.inject(1, rawForCelsius(35));
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 20, a.readCelsius());
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 35, b.readCelsius());
}
int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_initial_missing_sample_is_nan);
    RUN_TEST(test_constant_sample_and_metadata);
    RUN_TEST(test_sequence_deduplication);
    RUN_TEST(test_rolling_average_replaces_old_samples);
    RUN_TEST(test_invalid_voltage_does_not_replace_average);
    RUN_TEST(test_channels_are_independent);
    return UNITY_END();
}
