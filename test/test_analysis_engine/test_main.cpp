#include <unity.h>
#include "AnalysisEngine.h"
// Compile the actual production engine in the native test binary.
#include "../../src/AnalysisEngine.cpp"

void test_boundaries() {
    TEST_ASSERT_EQUAL_UINT32(1200, AnalysisEngine::secondsFromTicks(40));
    TEST_ASSERT_FALSE(AnalysisEngine::validHighTicks(6));
    TEST_ASSERT_TRUE(AnalysisEngine::validHighTicks(100));
    TEST_ASSERT_EQUAL_INT((int)AnalysisEngine::Level::Maint, (int)AnalysisEngine::classify(180,40));
    TEST_ASSERT_EQUAL_INT((int)AnalysisEngine::Level::Low, (int)AnalysisEngine::classify(181,40));
    TEST_ASSERT_EQUAL_INT((int)AnalysisEngine::Level::Low, (int)AnalysisEngine::classify(1199,40));
    TEST_ASSERT_EQUAL_INT((int)AnalysisEngine::Level::High, (int)AnalysisEngine::classify(1200,40));
    TEST_ASSERT_EQUAL_INT((int)AnalysisEngine::Level::High, (int)AnalysisEngine::classify(3000,100));
}
void test_empty() {
    AnalysisTotals t = AnalysisEngine::calculate(nullptr,0,0,3600,40,10);
    TEST_ASSERT_EQUAL_UINT64(0,t.on);
}
void test_cycle_reconstruction() {
    const AnalysisEvent events[] = {
        {100,60,0,AnalysisEventType::HEATING},
        {160,60,0,AnalysisEventType::HEATING},
        {220,10,0,AnalysisEventType::OFF},
        {230,1080,0,AnalysisEventType::HEATING},
        {1310,100,0,AnalysisEventType::OFF}
    };
    AnalysisTotals t = AnalysisEngine::calculate(events,5,0,3600,40,10);
    TEST_ASSERT_EQUAL_UINT64(1210,t.high);
    TEST_ASSERT_EQUAL_UINT64(0,t.low);
    TEST_ASSERT_EQUAL_UINT64(0,t.maint);
    TEST_ASSERT_EQUAL_UINT64(1210,t.on);
}
void test_long_off_splits_cycles() {
    const AnalysisEvent events[] = {
        {0,120,0,AnalysisEventType::HEATING},
        {120,60,0,AnalysisEventType::OFF},
        {180,600,0,AnalysisEventType::HEATING}
    };
    AnalysisTotals t = AnalysisEngine::calculate(events,3,0,3600,40,10);
    TEST_ASSERT_EQUAL_UINT64(120,t.maint);
    TEST_ASSERT_EQUAL_UINT64(600,t.low);
    TEST_ASSERT_EQUAL_UINT64(720,t.on);
    TEST_ASSERT_FLOAT_WITHIN(0.001f,20.0f,t.onPercent);
}
void test_calibration() {
    const AnalysisEvent events[] = {
        {0,600,0,AnalysisEventType::HEATING},
        {600,100,0,AnalysisEventType::OFF},
        {700,720,0,AnalysisEventType::HEATING},
        {1420,100,0,AnalysisEventType::OFF},
        {1520,1800,0,AnalysisEventType::HEATING},
        {3320,100,0,AnalysisEventType::OFF},
        {3420,2100,0,AnalysisEventType::HEATING}
    };
    int ticks=40;
    TEST_ASSERT_TRUE(AnalysisEngine::calibrate(events,7,10,ticks));
    TEST_ASSERT_TRUE(ticks>6 && ticks<100);
    TEST_ASSERT_FALSE(AnalysisEngine::calibrate(events,1,10,ticks));
}
int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_boundaries);
    RUN_TEST(test_empty);
    RUN_TEST(test_cycle_reconstruction);
    RUN_TEST(test_long_off_splits_cycles);
    RUN_TEST(test_calibration);
    return UNITY_END();
}
