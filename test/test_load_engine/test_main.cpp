#include <unity.h>
#include "LoadEngine.h"
#include "../../src/LoadEngine.cpp"
void test_event_scores() {
    LoadEngine engine;
    for (uint32_t t=1000;t<1240;t+=10) engine.feed(t,5,35,25);
    engine.feed(1240,0,35,25);
    LoadEvent e; TEST_ASSERT_TRUE(engine.takeCompleted(e));
    TEST_ASSERT_EQUAL_UINT32(240,e.seconds);
    TEST_ASSERT_FLOAT_WITHIN(0.01f,50,e.loadPct);
    TEST_ASSERT_FLOAT_WITHIN(0.01f,2,e.fullLoadMinutes);
    TEST_ASSERT_FLOAT_WITHIN(0.001f,0.5f,e.efficiencyIndex);
    TEST_ASSERT_EQUAL_UINT32(120,e.efficiencySeconds);
}
void test_off_threshold_and_missing_efficiency() {
    LoadEngine engine;
    engine.feed(1000,0.1f,20,20);
    TEST_ASSERT_FALSE(engine.active());
    engine.feed(1010,5,20,20);
    TEST_ASSERT_TRUE(engine.active());
    engine.feed(1020,0.1f,20,20);
    LoadEvent e; TEST_ASSERT_FALSE(engine.takeCompleted(e));
}
void test_gap_closes_event() {
    LoadEngine engine;
    for(uint32_t t=1000;t<1040;t+=10) engine.feed(t,5,30,20);
    engine.feed(1100,5,30,20);
    LoadEvent e;TEST_ASSERT_TRUE(engine.takeCompleted(e));
    TEST_ASSERT_EQUAL_UINT32(40,e.seconds);
    TEST_ASSERT_TRUE(engine.active());
}
int main(int,char**) {
    UNITY_BEGIN();
    RUN_TEST(test_event_scores);
    RUN_TEST(test_off_threshold_and_missing_efficiency);
    RUN_TEST(test_gap_closes_event);
    return UNITY_END();
}
