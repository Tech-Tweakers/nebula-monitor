#pragma once
// === CYD (ESP32-2432S028R) — ILI9341 ===
#define ILI9341_DRIVER

// TFT no HSPI (como no guia RNT)
#define TFT_MOSI 13
#define TFT_MISO 12
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST   -1     // se o teu não usa RST, troque para -1

#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  20000000
#define TFT_RGB_ORDER       TFT_BGR

// Só pra silenciar aviso interno (não usamos touch do TFT_eSPI)
#define TOUCH_CS -1
