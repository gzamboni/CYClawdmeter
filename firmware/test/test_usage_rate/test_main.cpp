#include <stdint.h>
#include <unity.h>

#include "usage_rate.h"

static uint32_t fake_now_ms = 0;

uint32_t millis(void) {
    return fake_now_ms;
}

static void sample_at(uint32_t ms, float pct) {
    fake_now_ms = ms;
    usage_rate_sample(pct);
}

static int group_for_delta(float pct_delta, uint32_t span_ms = 240000UL) {
    usage_rate_reset();
    sample_at(0, 10.0f);
    sample_at(span_ms, 10.0f + pct_delta);
    return usage_rate_group();
}

void setUp(void) {
    fake_now_ms = 0;
    usage_rate_reset();
}

void tearDown(void) {
}

void test_defaults_to_idle_until_window_is_long_enough(void) {
    sample_at(0, 0.0f);
    sample_at(60000UL, 10.0f);

    TEST_ASSERT_EQUAL_INT(0, usage_rate_group());
}

void test_rate_groups_match_thresholds_after_minimum_window(void) {
    TEST_ASSERT_EQUAL_INT(0, group_for_delta(0.36f));  // 0.09 %/min
    TEST_ASSERT_EQUAL_INT(1, group_for_delta(0.44f));  // safely above 0.10 %/min
    TEST_ASSERT_EQUAL_INT(1, group_for_delta(0.79f));  // just under active
    TEST_ASSERT_EQUAL_INT(2, group_for_delta(0.80f));  // 0.20 %/min
    TEST_ASSERT_EQUAL_INT(2, group_for_delta(1.31f));  // just under heavy
    TEST_ASSERT_EQUAL_INT(3, group_for_delta(1.40f));  // safely above 0.33 %/min
}

void test_ring_buffer_uses_recent_window(void) {
    for (int i = 0; i < 8; i++) {
        sample_at((uint32_t)i * 60000UL, (float)i * 0.5f);
    }

    TEST_ASSERT_EQUAL_INT(3, usage_rate_group());
}

void test_session_reset_drop_restarts_tracking(void) {
    sample_at(0, 80.0f);
    sample_at(240000UL, 82.0f);
    TEST_ASSERT_EQUAL_INT(3, usage_rate_group());

    sample_at(300000UL, 10.0f);
    TEST_ASSERT_EQUAL_INT(0, usage_rate_group());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_defaults_to_idle_until_window_is_long_enough);
    RUN_TEST(test_rate_groups_match_thresholds_after_minimum_window);
    RUN_TEST(test_ring_buffer_uses_recent_window);
    RUN_TEST(test_session_reset_drop_restarts_tracking);
    return UNITY_END();
}
