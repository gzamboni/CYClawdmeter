#include "brightness.h"
#include "idle.h"
#include <Preferences.h>
#include <Arduino.h>

// Four-step ramp retained for the physical PWR-button shortcut. The on-screen
// slider stores exact PWM values in the same NVS namespace.
static const uint8_t LEVELS[] = {64, 128, 200, 255};
#define LEVELS_COUNT (sizeof(LEVELS) / sizeof(LEVELS[0]))
#define DEFAULT_IDX  2

static uint8_t cur_idx = DEFAULT_IDX;
static uint8_t cur_level = LEVELS[DEFAULT_IDX];

static uint8_t nearest_level_idx(uint8_t level) {
    uint8_t best = 0;
    uint8_t best_delta = 255;
    for (uint8_t i = 0; i < LEVELS_COUNT; i++) {
        uint8_t delta = (level > LEVELS[i]) ? (level - LEVELS[i]) : (LEVELS[i] - level);
        if (delta < best_delta) {
            best = i;
            best_delta = delta;
        }
    }
    return best;
}

static void save_level(uint8_t level) {
    Preferences prefs;
    prefs.begin("clawdmeter", false);
    prefs.putUChar("brt", level);
    prefs.putUChar("brt_idx", nearest_level_idx(level));
    prefs.end();
}

void brightness_init(void) {
    Preferences prefs;
    prefs.begin("clawdmeter", true);
    uint8_t saved_level = prefs.getUChar("brt", 0xFF);
    uint8_t saved_idx = prefs.getUChar("brt_idx", 0xFF);
    prefs.end();

    if (saved_level != 0xFF) {
        cur_level = saved_level;
        cur_idx = nearest_level_idx(cur_level);
    } else if (saved_idx < LEVELS_COUNT) {
        cur_idx = saved_idx;
        cur_level = LEVELS[cur_idx];
    }
    idle_set_awake_brightness(cur_level);
    Serial.printf("Brightness init: level=%u (idx=%u)\n", cur_level, cur_idx);
}

void brightness_apply(uint8_t level) {
    cur_level = level;
    cur_idx = nearest_level_idx(cur_level);
    idle_set_awake_brightness(cur_level);
}

void brightness_set(uint8_t level) {
    brightness_apply(level);
    save_level(cur_level);
    Serial.printf("Brightness set: level=%u (idx=%u)\n", cur_level, cur_idx);
}

void brightness_cycle(void) {
    cur_idx = (cur_idx + 1) % LEVELS_COUNT;
    cur_level = LEVELS[cur_idx];
    save_level(cur_level);
    idle_set_awake_brightness(cur_level);
    Serial.printf("Brightness cycled: level=%u (idx=%u)\n", cur_level, cur_idx);
}

uint8_t brightness_get(void) {
    return cur_level;
}
