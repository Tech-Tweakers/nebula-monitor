#pragma once
// === CYD (ESP32-2432S028R) — ST7789 ===
// This file matches platformio.ini build flags for consistency
// Note: USER_SETUP_LOADED=1 in platformio.ini overrides this file

#define ST7789_DRIVER
#define TFT_WIDTH 240
#define TFT_HEIGHT 320
#define TFT_RGB_ORDER TFT_BGR
#define TFT_INVERSION_ON 1

// TFT pins (HSPI)
#define TFT_MOSI 13
#define TFT_MISO 12
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  -1     // Not connected

// Backlight
#define TFT_BL 27
#define TFT_BACKLIGHT_ON HIGH

// Touch controller (XPT2046 on shared HSPI)
#define TOUCH_CS 33

// SD Card (VSPI)
#define SD_CS 5

// SPI settings
#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY 2500000
#define USE_HSPI_PORT 1

// Font loading
#define LOAD_GLCD 1
#define LOAD_FONT2 1
#define LOAD_FONT4 1
#define LOAD_FONT6 1
#define LOAD_FONT7 1
#define LOAD_FONT8 1
#define LOAD_GFXFF 1
#define SMOOTH_FONT 1
