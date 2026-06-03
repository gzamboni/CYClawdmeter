#include "../../hal/touch_hal.h"
#include "board.h"
#include <Arduino.h>
#include <SPI.h>

// XPT2046 SPI commands for 12-bit conversion.
#define XPT_CMD_X  0xD0
#define XPT_CMD_Y  0x90
#define XPT_CMD_Z1 0xB0   // pressure Z1 (0 = no touch, ~500-2000 = pressed)
#define XPT_CMD_Z2 0xC0   // pressure Z2

// Minimum Z1 reading accepted as a real touch (filters electrical noise).
// Raise this value if phantom presses persist; lower it if taps are missed.
#define XPT_Z1_MIN 80

// Default calibration for ESP32-2432S028R boards.
// If your panel drifts, tune these 4 values.
#define XPT_X_MIN  240
#define XPT_X_MAX  3850
#define XPT_Y_MIN  220
#define XPT_Y_MAX  3820

static SPIClass tp_spi(HSPI);

static volatile bool     touch_data_ready = false;
static volatile bool     touch_pressed = false;
static volatile uint16_t touch_x = 0;
static volatile uint16_t touch_y = 0;

static void IRAM_ATTR touch_isr(void) {
    touch_data_ready = true;
}

static uint16_t xpt_read12(uint8_t cmd) {
    tp_spi.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
    digitalWrite(TP_CS, LOW);
    tp_spi.transfer(cmd);
    uint16_t hi = tp_spi.transfer(0x00);
    uint16_t lo = tp_spi.transfer(0x00);
    digitalWrite(TP_CS, HIGH);
    tp_spi.endTransaction();
    return (uint16_t)(((hi << 8) | lo) >> 3) & 0x0FFF;
}

static uint16_t clamp_u16(long v, uint16_t lo, uint16_t hi) {
    if (v < (long)lo) return lo;
    if (v > (long)hi) return hi;
    return (uint16_t)v;
}

void touch_hal_init(void) {
    tp_spi.begin(TP_SCLK, TP_MISO, TP_MOSI, TP_CS);
    pinMode(TP_CS, OUTPUT);
    digitalWrite(TP_CS, HIGH);

    pinMode(TP_INT, INPUT_PULLUP);
    attachInterrupt(TP_INT, touch_isr, FALLING);
}

void touch_hal_read(uint16_t* x, uint16_t* y, bool* pressed) {
    // Release path: INT is HIGH and no pending ISR data → finger is off the panel.
    // Without this the XPT2046 never clears touch_pressed after a lift, causing
    // the LVGL indev to stay "pressed" and fire phantom click events.
    if (!touch_data_ready && digitalRead(TP_INT) == HIGH) {
        touch_pressed = false;
        *x = touch_x;
        *y = touch_y;
        *pressed = false;
        return;
    }

    if (touch_data_ready || digitalRead(TP_INT) == LOW) {
        touch_data_ready = false;

        // Read Z pressure first — filters out electrical noise that pulls INT
        // low without an actual finger on the panel.
        uint16_t z1 = xpt_read12(XPT_CMD_Z1);
        if (z1 < XPT_Z1_MIN) {
            // No real pressure → treat as release and drain any pending ISR flag.
            touch_pressed = false;
            *x = touch_x;
            *y = touch_y;
            *pressed = false;
            return;
        }

        // Median-like sampling: 3 reads, keep center value to reduce jitter.
        uint16_t xr0 = xpt_read12(XPT_CMD_X);
        uint16_t yr0 = xpt_read12(XPT_CMD_Y);
        uint16_t xr1 = xpt_read12(XPT_CMD_X);
        uint16_t yr1 = xpt_read12(XPT_CMD_Y);
        uint16_t xr2 = xpt_read12(XPT_CMD_X);
        uint16_t yr2 = xpt_read12(XPT_CMD_Y);

        uint16_t xr = (uint16_t)((xr0 + xr1 + xr2) / 3);
        uint16_t yr = (uint16_t)((yr0 + yr1 + yr2) / 3);

        if (xr < 50 || yr < 50 || xr > 4090 || yr > 4090) {
            touch_pressed = false;
        } else {
            // Map raw ADC → screen pixels. The XPT2046 axes on the CYD board
            // run opposite to the ILI9341 display rotation, so both ranges are
            // inverted (max→0, min→width/height-1) to align touch and display
            // coordinate systems.
            long mapped_x = map((long)xr, XPT_X_MIN, XPT_X_MAX, LCD_WIDTH - 1,  0);
            long mapped_y = map((long)yr, XPT_Y_MIN, XPT_Y_MAX, LCD_HEIGHT - 1, 0);

            touch_x = clamp_u16(mapped_x, 0, LCD_WIDTH - 1);
            touch_y = clamp_u16(mapped_y, 0, LCD_HEIGHT - 1);
            touch_pressed = true;
        }
    }

    *x = touch_x;
    *y = touch_y;
    *pressed = touch_pressed;
}
