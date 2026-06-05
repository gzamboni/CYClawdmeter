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

// ── existing tests ────────────────────────────────────────────────────────────

void test_defaults_to_idle_until_window_is_long_enough(void) {
    sample_at(0, 0.0f);
    sample_at(60000UL, 10.0f);

    TEST_ASSERT_EQUAL_INT(0, usage_rate_group());
}

void test_rate_groups_match_thresholds_after_minimum_window(void) {
    TEST_ASSERT_EQUAL_INT(0, group_for_delta(0.36f));  // 0.09 %/min  → idle
    TEST_ASSERT_EQUAL_INT(1, group_for_delta(0.44f));  // 0.11 %/min  → normal
    TEST_ASSERT_EQUAL_INT(1, group_for_delta(0.79f));  // just under active
    TEST_ASSERT_EQUAL_INT(2, group_for_delta(0.80f));  // 0.20 %/min  → active
    TEST_ASSERT_EQUAL_INT(2, group_for_delta(1.31f));  // just under heavy
    TEST_ASSERT_EQUAL_INT(3, group_for_delta(1.40f));  // 0.35 %/min  → heavy
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

// ── new tests ─────────────────────────────────────────────────────────────────

// count == 1: group() returns 0 regardless of the value.
void test_one_sample_always_idle(void) {
    sample_at(500000UL, 99.0f);
    TEST_ASSERT_EQUAL_INT(0, usage_rate_group());
}

// dt exactly == MIN_WINDOW_MS: guard is `dt < MIN_WINDOW_MS`, so rate IS computed.
void test_window_at_exact_minimum_gives_real_rate(void) {
    // span=240000, dp=5.0 → rate=1.25 %/min → heavy
    TEST_ASSERT_EQUAL_INT(3, group_for_delta(5.0f, 240000UL));
}

// dt one ms below MIN_WINDOW_MS: still reports idle.
void test_window_one_ms_short_of_minimum_is_idle(void) {
    TEST_ASSERT_EQUAL_INT(0, group_for_delta(5.0f, 239999UL));
}

// pct unchanged over time: dp=0, rate=0 → idle.
void test_zero_delta_pct_is_idle(void) {
    sample_at(0, 20.0f);
    sample_at(300000UL, 20.0f);
    TEST_ASSERT_EQUAL_INT(0, usage_rate_group());
}

// pct decreases by < 5.0: session reset NOT triggered, negative dp clamped to 0
// by `if (dp < 0) dp = 0`, so rate=0 → idle.
void test_small_pct_decrease_clamps_rate_to_idle(void) {
    sample_at(0, 30.0f);
    sample_at(240000UL, 25.1f);  // drop of 4.9 — below the 5.0 reset threshold
    TEST_ASSERT_EQUAL_INT(0, usage_rate_group());
}

// Drop of exactly 5.0: condition is `pct + 5.0 < prev`, so 25.0+5.0=30.0 is
// NOT < 30.0 — reset not triggered, ring keeps all samples.
void test_session_reset_not_triggered_by_exact_five_drop(void) {
    sample_at(0,        10.0f);
    sample_at(240000UL, 30.0f);  // heavy rate from here
    TEST_ASSERT_EQUAL_INT(3, usage_rate_group());

    sample_at(300000UL, 25.0f);  // exactly 5.0 drop — no reset
    // oldest=(0,10), latest=(300k,25): dp=15, rate=3.0 %/min → still heavy
    TEST_ASSERT_EQUAL_INT(3, usage_rate_group());
}

// Drop of 5.001: reset IS triggered, ring cleared; only new sample remains → idle.
void test_session_reset_triggered_by_drop_above_five(void) {
    sample_at(0,        10.0f);
    sample_at(240000UL, 30.0f);
    TEST_ASSERT_EQUAL_INT(3, usage_rate_group());

    sample_at(300000UL, 24.9f);  // 30.0 - 24.9 = 5.1 — reset fires
    TEST_ASSERT_EQUAL_INT(0, usage_rate_group());
}

// After RING_SIZE samples the ring is full. The (RING_SIZE+1)-th sample wraps
// slot 0. oldest_idx must return slot 1 (second-oldest sample), not slot 0
// (which now holds the newest write). If oldest_idx were wrong and returned 0,
// oldest == latest → dt == 0 → idle; with correct logic dt = 300 s → heavy.
void test_ring_wrap_oldest_slot_used_after_overflow(void) {
    // 6 samples at 60 s intervals, pct=0..5 → rate=1 %/min → heavy
    for (int i = 0; i < 6; i++) {
        sample_at((uint32_t)i * 60000UL, (float)i);
    }
    TEST_ASSERT_EQUAL_INT(3, usage_rate_group());

    // 7th sample overwrites slot 0. oldest must now be slot 1 = (60k, 1.0).
    // latest = slot 0 = (360k, 6.0). dt=300k, dp=5.0, rate=1.0 %/min → heavy.
    sample_at(360000UL, 6.0f);
    TEST_ASSERT_EQUAL_INT(3, usage_rate_group());
}

// After reset(), a fresh sample immediately after still shows idle.
void test_after_reset_single_new_sample_is_idle(void) {
    sample_at(0,        10.0f);
    sample_at(240000UL, 20.0f);
    TEST_ASSERT_EQUAL_INT(3, usage_rate_group());

    usage_rate_reset();
    sample_at(300000UL, 5.0f);
    TEST_ASSERT_EQUAL_INT(0, usage_rate_group());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_defaults_to_idle_until_window_is_long_enough);
    RUN_TEST(test_rate_groups_match_thresholds_after_minimum_window);
    RUN_TEST(test_ring_buffer_uses_recent_window);
    RUN_TEST(test_session_reset_drop_restarts_tracking);
    RUN_TEST(test_one_sample_always_idle);
    RUN_TEST(test_window_at_exact_minimum_gives_real_rate);
    RUN_TEST(test_window_one_ms_short_of_minimum_is_idle);
    RUN_TEST(test_zero_delta_pct_is_idle);
    RUN_TEST(test_small_pct_decrease_clamps_rate_to_idle);
    RUN_TEST(test_session_reset_not_triggered_by_exact_five_drop);
    RUN_TEST(test_session_reset_triggered_by_drop_above_five);
    RUN_TEST(test_ring_wrap_oldest_slot_used_after_overflow);
    RUN_TEST(test_after_reset_single_new_sample_is_idle);
    return UNITY_END();
}
