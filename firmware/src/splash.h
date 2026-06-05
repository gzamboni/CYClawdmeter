#pragma once
#include <stdint.h>
#include <lvgl.h>

// Initialize splash module. Creates the canvas widget inside `parent` and
// allocates the 480x480 pixel buffer (PSRAM).
void splash_init(lv_obj_t *parent);

// Advance animation frame if hold time elapsed. Call from main loop.
void splash_tick(void);

// Cycle to the next animation in the catalog.
void splash_next(void);

// Show/hide the splash container.
void splash_show(void);
void splash_hide(void);

// Pick the next animation matching the current usage-rate group.
// Called automatically by splash_show(); also exposed so other modules can
// trigger a re-pick when the rate group changes mid-display.
void splash_pick_for_current_rate(void);

// True when splash is currently rendering (used to gate re-picks).
bool splash_is_active(void);

// True if the current frame or animation changed since the last call (clears on read).
// ui.cpp uses this to re-render the mini logo canvas only when needed.
bool splash_mini_dirty(void);

// Milliseconds timestamp of the last animation pick (for usage-screen rotation pacing).
uint32_t splash_last_pick_ms(void);

// Root container (so ui.cpp can attach a click event).
lv_obj_t* splash_get_root(void);

// Grid size (cells per row/column) — exposed so callers can size output buffers.
#define SPLASH_GRID 20

// Render the current animation frame into an arbitrary RGB565 buffer at the
// given cell_size (pixels per grid cell).  E.g. cell_size=2 → 40×40 output.
// Buffer must hold at least (SPLASH_GRID * cell_size)^2 uint16_t entries.
// Safe to call at any time (frame counter advances via splash_tick).
void splash_render_small(uint16_t* buf, int cell_size);
