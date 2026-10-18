#include <unity.h>
#include "CycleClassification.h"
using namespace CycleClassification;
void test_slider_mapping() {
    TEST_ASSERT_EQUAL_UINT32(0, secondsFromTicks(0));
    TEST_ASSERT_EQUAL_UINT32(30, secondsFromTicks(1));
    TEST_ASSERT_EQUAL_UINT32(1200, secondsFromTicks(40));
    TEST_ASSERT_EQUAL_UINT32(3000, secondsFromTicks(100));
}
void test_boundaries() {
    TEST_ASSERT_EQUAL_INT((int)Level::MAINT, (int)classify(180,40));
    TEST_ASSERT_EQUAL_INT((int)Level::LOW, (int)classify(181,40));
    TEST_ASSERT_EQUAL_INT((int)Level::LOW, (int)classify(1199,40));
    TEST_ASSERT_EQUAL_INT((int)Level::HIGH, (int)classify(1200,40));
    TEST_ASSERT_EQUAL_INT((int)Level::HIGH, (int)classify(3000,100));
}
void test_threshold_validation() {
    TEST_ASSERT_FALSE(validHighTicks(6));
    TEST_ASSERT_TRUE(validHighTicks(7));
    TEST_ASSERT_TRUE(validHighTicks(100));
    TEST_ASSERT_FALSE(validHighTicks(101));
}
void test_off_gap() {
    TEST_ASSERT_TRUE(canBridgeOff(10,10));
    TEST_ASSERT_FALSE(canBridgeOff(11,10));
}
int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_slider_mapping);
    RUN_TEST(test_boundaries);
    RUN_TEST(test_threshold_validation);
    RUN_TEST(test_off_gap);
    return UNITY_END();
}
