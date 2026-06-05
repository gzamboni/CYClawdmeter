#pragma once
#include <stdint.h>

// User-controlled display brightness, persisted to NVS. The middle (PWR)
// button short-press still cycles coarse levels via brightness_cycle(), while
// the on-screen menu can set any 0..255 value via brightness_set().
// idle owns the actual panel brightness, so this routes the chosen level
// through idle_set_awake_brightness().
void    brightness_init(void);    // load saved level from NVS and apply
void    brightness_apply(uint8_t level); // apply exact PWM level without persisting
void    brightness_set(uint8_t level); // save and apply exact PWM level
void    brightness_cycle(void);   // advance to next level, save, apply
uint8_t brightness_get(void);     // current PWM level (0..255)
