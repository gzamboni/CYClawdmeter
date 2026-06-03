#include "../../hal/display_hal.h"
#include "board.h"
#include <Arduino.h>
#include <Arduino_GFX_Library.h>

static Arduino_DataBus* bus = nullptr;
static Arduino_ILI9341* gfx = nullptr;

static bool level_to_on(uint8_t level) {
    return level > 0;
}

void display_hal_init(void) {
    bus = new Arduino_ESP32SPI(
        LCD_DC, LCD_CS, LCD_SCLK, LCD_MOSI, LCD_MISO);
    // ILI9341 constructor: (bus, rst, rotation) — 1 = landscape 90°
    gfx = new Arduino_ILI9341(bus, LCD_RESET, 1);
}

void display_hal_begin(void) {
    gfx->begin();
    gfx->fillScreen(0x0000);
    digitalWrite(LCD_BL, HIGH);
}

void display_hal_set_brightness(uint8_t level) {
    digitalWrite(LCD_BL, level_to_on(level) ? HIGH : LOW);
}

void display_hal_fill_screen(uint16_t color) {
    if (gfx) gfx->fillScreen(color);
}

void display_hal_draw_bitmap(int32_t x, int32_t y, int32_t w, int32_t h,
                             const uint16_t* pixels) {
    if (gfx) gfx->draw16bitRGBBitmap(x, y, (uint16_t*)pixels, w, h);
}

void display_hal_tick(void) {
    // No rotation handling on this board.
}

void display_hal_round_area(int32_t* x1, int32_t* y1, int32_t* x2, int32_t* y2) {
    *x1 = *x1 & ~1;
    *y1 = *y1 & ~1;
    *x2 = *x2 | 1;
    *y2 = *y2 | 1;
}
