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

void test_null_and_invalid_windows() {
    const AnalysisEvent e[] = {{0, 600, 0, AnalysisEventType::HEATING}};
    TEST_ASSERT_EQUAL_UINT64(0, AnalysisEngine::calculate(nullptr, 1, 0, 3600, 40, 10).on);
    TEST_ASSERT_EQUAL_UINT64(0, AnalysisEngine::calculate(e, 1, 3600, 3600, 40, 10).on);
    TEST_ASSERT_EQUAL_UINT64(0, AnalysisEngine::calculate(e, 1, 3600, 100, 40, 10).on);
    TEST_ASSERT_EQUAL_UINT64(0, AnalysisEngine::calculate(e, 1, 0, 3600, 6, 10).on);
}
void test_classification_exact_edges() {
    TEST_ASSERT_EQUAL_INT((int)AnalysisEngine::Level::Maint, (int)AnalysisEngine::classify(0, 40));
    TEST_ASSERT_EQUAL_INT((int)AnalysisEngine::Level::Maint, (int)AnalysisEngine::classify(180, 40));
    TEST_ASSERT_EQUAL_INT((int)AnalysisEngine::Level::Low, (int)AnalysisEngine::classify(181, 40));
    TEST_ASSERT_EQUAL_INT((int)AnalysisEngine::Level::Low, (int)AnalysisEngine::classify(1199, 40));
    TEST_ASSERT_EQUAL_INT((int)AnalysisEngine::Level::High, (int)AnalysisEngine::classify(1200, 40));
    TEST_ASSERT_FALSE(AnalysisEngine::validHighTicks(101));
    TEST_ASSERT_EQUAL_UINT32(0, AnalysisEngine::secondsFromTicks(0));
}
void test_window_clips_start_and_end() {
    const AnalysisEvent e[] = {{100, 1000, 0, AnalysisEventType::HEATING}};
    AnalysisTotals t = AnalysisEngine::calculate(e, 1, 300, 800, 40, 10);
    TEST_ASSERT_EQUAL_UINT64(500, t.low);
    TEST_ASSERT_EQUAL_UINT64(0, t.off);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, t.onPercent);
}
void test_cycle_level_uses_full_duration_when_clipped() {
    const AnalysisEvent e[] = {{100, 1300, 0, AnalysisEventType::HEATING}};
    AnalysisTotals t = AnalysisEngine::calculate(e, 1, 1000, 1200, 40, 10);
    TEST_ASSERT_EQUAL_UINT64(200, t.high);
}
void test_zero_length_events_ignored() {
    const AnalysisEvent e[] = {{0, 0, 0, AnalysisEventType::HEATING}, {100, 300, 0, AnalysisEventType::HEATING}};
    AnalysisTotals t = AnalysisEngine::calculate(e, 2, 0, 1000, 40, 10);
    TEST_ASSERT_EQUAL_UINT64(300, t.low);
}
void test_exact_off_bridge_boundary() {
    const AnalysisEvent e[] = {
        {0, 200, 0, AnalysisEventType::HEATING},
        {200, 10, 0, AnalysisEventType::OFF},
        {210, 200, 0, AnalysisEventType::HEATING}
    };
    AnalysisTotals t = AnalysisEngine::calculate(e, 3, 0, 1000, 40, 10);
    // Current behavior: bridged OFF time is included in cycle duration and ON totals.
    TEST_ASSERT_EQUAL_UINT64(410, t.low);
}
void test_off_one_second_beyond_bridge() {
    const AnalysisEvent e[] = {
        {0, 200, 0, AnalysisEventType::HEATING},
        {200, 11, 0, AnalysisEventType::OFF},
        {211, 200, 0, AnalysisEventType::HEATING}
    };
    AnalysisTotals t = AnalysisEngine::calculate(e, 3, 0, 1000, 40, 10);
    TEST_ASSERT_EQUAL_UINT64(400, t.low);
}
void test_multiple_short_gaps_bridge() {
    const AnalysisEvent e[] = {
        {0, 200, 0, AnalysisEventType::HEATING},
        {200, 5, 0, AnalysisEventType::OFF},
        {205, 200, 0, AnalysisEventType::HEATING},
        {405, 5, 0, AnalysisEventType::OFF},
        {410, 200, 0, AnalysisEventType::HEATING}
    };
    AnalysisTotals t = AnalysisEngine::calculate(e, 5, 0, 1000, 40, 10);
    TEST_ASSERT_EQUAL_UINT64(610, t.low);
}
void test_overlapping_heating_does_not_double_count() {
    const AnalysisEvent e[] = {
        {100, 300, 0, AnalysisEventType::HEATING},
        {200, 300, 0, AnalysisEventType::HEATING}
    };
    AnalysisTotals t = AnalysisEngine::calculate(e, 2, 0, 1000, 40, 10);
    TEST_ASSERT_EQUAL_UINT64(400, t.low);
}
void test_off_only_window() {
    const AnalysisEvent e[] = {{0, 300, 0, AnalysisEventType::OFF}};
    AnalysisTotals t = AnalysisEngine::calculate(e, 1, 0, 1000, 40, 10);
    TEST_ASSERT_EQUAL_UINT64(0, t.on);
    TEST_ASSERT_EQUAL_UINT64(1000, t.off);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, t.offPercent);
}
void test_mixed_percentages() {
    const AnalysisEvent e[] = {
        {0, 120, 0, AnalysisEventType::HEATING},
        {120, 100, 0, AnalysisEventType::OFF},
        {220, 300, 0, AnalysisEventType::HEATING},
        {520, 100, 0, AnalysisEventType::OFF},
        {620, 1300, 0, AnalysisEventType::HEATING}
    };
    AnalysisTotals t = AnalysisEngine::calculate(e, 5, 0, 2000, 40, 10);
    TEST_ASSERT_EQUAL_UINT64(120, t.maint);
    TEST_ASSERT_EQUAL_UINT64(300, t.low);
    TEST_ASSERT_EQUAL_UINT64(1300, t.high);
    TEST_ASSERT_EQUAL_UINT64(1720, t.on);
    TEST_ASSERT_EQUAL_UINT64(280, t.off);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 86.0f, t.onPercent);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, t.maintPercent + t.lowPercent + t.highPercent);
}
void test_calibration_insufficient_cycles_preserves_setting() {
    const AnalysisEvent e[] = {
        {0, 600, 0, AnalysisEventType::HEATING},
        {700, 700, 0, AnalysisEventType::HEATING},
        {1500, 800, 0, AnalysisEventType::HEATING}
    };
    int ticks = 40;
    TEST_ASSERT_FALSE(AnalysisEngine::calibrate(e, 3, 10, ticks));
    TEST_ASSERT_EQUAL_INT(40, ticks);
}
void test_calibration_single_cluster_rejected() {
    const AnalysisEvent e[] = {
        {0, 600, 0, AnalysisEventType::HEATING},
        {700, 600, 0, AnalysisEventType::HEATING},
        {1400, 600, 0, AnalysisEventType::HEATING},
        {2100, 600, 0, AnalysisEventType::HEATING}
    };
    int ticks = 40;
    TEST_ASSERT_FALSE(AnalysisEngine::calibrate(e, 4, 10, ticks));
    TEST_ASSERT_EQUAL_INT(40, ticks);
}
void test_calibration_ignores_maintenance_cycles() {
    const AnalysisEvent e[] = {
        {0, 120, 0, AnalysisEventType::HEATING},
        {200, 600, 0, AnalysisEventType::HEATING},
        {900, 720, 0, AnalysisEventType::HEATING},
        {1700, 1800, 0, AnalysisEventType::HEATING}
    };
    int ticks = 40;
    TEST_ASSERT_FALSE(AnalysisEngine::calibrate(e, 4, 10, ticks));
    TEST_ASSERT_EQUAL_INT(40, ticks);
}

void test_short_on_between_off_is_removed() {
    const AnalysisEvent e[] = {
        {0,100,0,AnalysisEventType::OFF},
        {100,20,0,AnalysisEventType::HEATING},
        {120,100,0,AnalysisEventType::OFF}
    };
    const auto t = AnalysisEngine::calculate(e,3,0,220,40,30);
    TEST_ASSERT_EQUAL_UINT64(0,t.on);
    TEST_ASSERT_EQUAL_UINT64(220,t.off);
}
void test_short_off_between_on_is_bridged() {
    const AnalysisEvent e[] = {
        {0,200,0,AnalysisEventType::HEATING},
        {200,20,0,AnalysisEventType::OFF},
        {220,200,0,AnalysisEventType::HEATING}
    };
    const auto t = AnalysisEngine::calculate(e,3,0,420,40,30);
    TEST_ASSERT_EQUAL_UINT64(420,t.low);
    TEST_ASSERT_EQUAL_UINT64(0,t.off);
}
void test_minimum_on_boundary_and_below() {
    const AnalysisEvent e[] = {
        {0,20,0,AnalysisEventType::HEATING},
        {20,100,0,AnalysisEventType::OFF},
        {120,30,0,AnalysisEventType::HEATING}
    };
    const auto t = AnalysisEngine::calculate(e,3,0,150,40,30);
    TEST_ASSERT_EQUAL_UINT64(30,t.maint);
    TEST_ASSERT_EQUAL_UINT64(120,t.off);
}
void test_long_off_separates_on_cycles() {
    const AnalysisEvent e[] = {
        {0,200,0,AnalysisEventType::HEATING},
        {200,40,0,AnalysisEventType::OFF},
        {240,200,0,AnalysisEventType::HEATING}
    };
    const auto t = AnalysisEngine::calculate(e,3,0,440,40,30);
    TEST_ASSERT_EQUAL_UINT64(400,t.low);
    TEST_ASSERT_EQUAL_UINT64(40,t.off);
}
int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_short_on_between_off_is_removed);
    RUN_TEST(test_short_off_between_on_is_bridged);
    RUN_TEST(test_minimum_on_boundary_and_below);
    RUN_TEST(test_long_off_separates_on_cycles);
    RUN_TEST(test_boundaries);
    RUN_TEST(test_empty);
    RUN_TEST(test_cycle_reconstruction);
    RUN_TEST(test_long_off_splits_cycles);
    RUN_TEST(test_calibration);
    RUN_TEST(test_null_and_invalid_windows);
    RUN_TEST(test_classification_exact_edges);
    RUN_TEST(test_window_clips_start_and_end);
    RUN_TEST(test_cycle_level_uses_full_duration_when_clipped);
    RUN_TEST(test_zero_length_events_ignored);
    RUN_TEST(test_exact_off_bridge_boundary);
    RUN_TEST(test_off_one_second_beyond_bridge);
    RUN_TEST(test_multiple_short_gaps_bridge);
    RUN_TEST(test_overlapping_heating_does_not_double_count);
    RUN_TEST(test_off_only_window);
    RUN_TEST(test_mixed_percentages);
    RUN_TEST(test_calibration_insufficient_cycles_preserves_setting);
    RUN_TEST(test_calibration_single_cluster_rejected);
    RUN_TEST(test_calibration_ignores_maintenance_cycles);
    return UNITY_END();
}
