#include <unity.h>
#include <cmath>
#include "TempSensor.h"

// Link the actual production sensor implementation against a native Arduino shim.
#include "../../src/TempSensor.cpp"

static int adcValue = 2048;
static int pinModeCalls = 0;
unsigned long millis() { return 0; }
int analogRead(int) { return adcValue; }
void pinMode(int, int) { ++pinModeCalls; }

static float expectedCelsius(int adc) {
    const float resistance = 10000.0f * adc / (4095.0f - adc);
    const float inverseKelvin = logf(resistance / 10000.0f) / 3950.0f + 1.0f / 298.15f;
    return 1.0f / inverseKelvin - 273.15f;
}
static void reset(int adc = 2048) { adcValue = adc; pinModeCalls = 0; }

void test_constant_adc_and_metadata() {
    reset();
    TemperatureSensor sensor("large", 35);
    TEST_ASSERT_EQUAL_STRING("large", sensor.name());
    TEST_ASSERT_EQUAL_INT(35, sensor.pin());
    TEST_ASSERT_EQUAL_INT(1, pinModeCalls);
    for (int i = 0; i < 80; ++i)
        TEST_ASSERT_FLOAT_WITHIN(0.002f, expectedCelsius(2048), sensor.readCelsius());
}
void test_nominal_thermistor_point() {
    reset(2047); // 2047.5 corresponds to 10K ohms, or 25 C.
    TemperatureSensor sensor("ambient", 34);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 25.0f, sensor.readCelsius());
}
void test_symmetric_outliers_trimmed() {
    reset();
    TemperatureSensor sensor("large", 35);
    // Exactly three high and three low values must be excluded from 60 samples.
    for (int i = 0; i < 54; ++i) sensor.readCelsius();
    adcValue = 1000;
    for (int i = 0; i < 3; ++i) sensor.readCelsius();
    adcValue = 3000;
    for (int i = 0; i < 3; ++i) sensor.readCelsius();
    TEST_ASSERT_FLOAT_WITHIN(0.002f, expectedCelsius(2048), sensor.readCelsius());
}
void test_sustained_step_fills_rolling_window() {
    reset();
    TemperatureSensor sensor("large", 35);
    adcValue = 2500;
    // Each new valid sample replaces one old sample; after 60 all are new.
    for (int i = 0; i < 60; ++i) sensor.readCelsius();
    TEST_ASSERT_FLOAT_WITHIN(0.002f, expectedCelsius(2500), sensor.readCelsius());
}
void test_invalid_samples_do_not_advance_window() {
    reset();
    TemperatureSensor sensor("large", 35);
    adcValue = 0;
    for (int i = 0; i < 80; ++i)
        TEST_ASSERT_FLOAT_WITHIN(0.002f, expectedCelsius(2048), sensor.readCelsius());
    adcValue = 4095;
    for (int i = 0; i < 80; ++i)
        TEST_ASSERT_FLOAT_WITHIN(0.002f, expectedCelsius(2048), sensor.readCelsius());
    adcValue = 2500;
    for (int i = 0; i < 60; ++i) sensor.readCelsius();
    TEST_ASSERT_FLOAT_WITHIN(0.002f, expectedCelsius(2500), sensor.readCelsius());
}
void test_invalid_initial_read_is_nan() {
    reset(0);
    TemperatureSensor sensor("large", 35);
    TEST_ASSERT_TRUE(std::isnan(sensor.readCelsius()));
}
int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_constant_adc_and_metadata);
    RUN_TEST(test_nominal_thermistor_point);
    RUN_TEST(test_symmetric_outliers_trimmed);
    RUN_TEST(test_sustained_step_fills_rolling_window);
    RUN_TEST(test_invalid_samples_do_not_advance_window);
    RUN_TEST(test_invalid_initial_read_is_nan);
    return UNITY_END();
}
