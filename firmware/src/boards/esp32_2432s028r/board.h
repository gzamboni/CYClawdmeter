#pragma once

// ESP32-2432S028R "Cheap Yellow Display" (CYD)
// 2.8" 240x320 ILI9341 SPI TFT + XPT2046 resistive touch.

#define BOARD_NAME           "ESP32-2432S028R (CYD)"

// ---- Display geometry ----
#define LCD_WIDTH            320
#define LCD_HEIGHT           240

// ---- LCD (ILI9341 on VSPI) ----
#define LCD_MOSI             13
#define LCD_MISO             GFX_NOT_DEFINED
#define LCD_SCLK             14
#define LCD_CS               15
#define LCD_DC               2
#define LCD_RESET            12
#define LCD_BL               21

// ---- Touch (XPT2046 on dedicated SPI bus) ----
#define TP_MOSI              32
#define TP_MISO              39
#define TP_SCLK              25
#define TP_CS                33
#define TP_INT               36

// ---- Optional I2C bus (not used on CYD path) ----
#define IIC_SDA              22
#define IIC_SCL              27

// ---- Buttons ----
#define BTN_BACK_GPIO        0     // BOOT button

// ---- Capability flags ----
#define BOARD_HAS_SECONDARY_BUTTON 0
#define BOARD_HAS_ROTATION         1
#define BOARD_HAS_IMU              0
#define BOARD_HAS_BATTERY          0
#define BOARD_HAS_IO_EXPANDER      0
