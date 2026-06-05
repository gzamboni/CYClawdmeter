#pragma once
#include <stdint.h>
#include "data.h"
#include "ble.h"

enum screen_t {
    SCREEN_SPLASH,
    SCREEN_USAGE,
    SCREEN_COUNT,
};

// Callbacks injected by main.cpp so ui.cpp doesn't depend on brightness/ble directly.
typedef void (*ui_action_cb_t)(void);
typedef void (*ui_brightness_set_cb_t)(uint8_t level);
typedef uint8_t (*ui_brightness_get_cb_t)(void);
void ui_set_brightness_cbs(ui_brightness_set_cb_t apply_cb,
                           ui_brightness_set_cb_t save_cb,
                           ui_brightness_get_cb_t get_cb);
void ui_set_pair_mode_cb(ui_action_cb_t cb);

void ui_init(void);
void ui_update(const UsageData* data);
void ui_tick_anim(void);
void ui_show_screen(screen_t screen);
void ui_toggle_splash(void);
screen_t ui_get_current_screen(void);
void ui_update_ble_status(ble_state_t state, const char* name, const char* mac);
void ui_update_battery(int percent, bool charging);
