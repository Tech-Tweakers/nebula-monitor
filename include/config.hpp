#pragma once

#ifndef LGFX_USE_V1
  #define LGFX_USE_V1
#endif
#ifndef LGFX_AUTODETECT
  #define LGFX_AUTODETECT
#endif

#include <LovyanGFX.hpp>
#if __has_include(<lgfx_user/LGFX_AUTODETECT.hpp>)
  #include <lgfx_user/LGFX_AUTODETECT.hpp>
#elif __has_include(<LGFX_AUTODETECT.hpp>)
  #include <LGFX_AUTODETECT.hpp>
#endif

// ---------- Hardware ----------
static constexpr uint8_t ROT    = 2;
static constexpr int     BL_PIN = 27;

// Touch (HSPI compartilhado com LCD ILI9341)
static constexpr int T_SCK  = 14;
static constexpr int T_MOSI = 13;
static constexpr int T_MISO = 12;
static constexpr int T_CS   = 33;
static constexpr int T_IRQ  = 36;

// Calibração bruta XPT2046
static constexpr int RAW_X_MIN = 200,  RAW_X_MAX = 3700;
static constexpr int RAW_Y_MIN = 240,  RAW_Y_MAX = 3800;

// ---------- Wi-Fi ----------
static constexpr const char* WIFI_SSID = "Polaris";
static constexpr const char* WIFI_PASS = "55548502";

// ---------- UI / Cores ----------
#define COL_BG    TFT_BLACK
#define COL_BAR   TFT_DARKGREY
#define COL_TXT   TFT_WHITE
#define COL_UP    0x07E0     // verde
#define COL_WARN  0xFFE0     // amarelo
#define COL_SLOW  0xFD20     // laranja
#define COL_DOWN  0xF800     // vermelho
#define COL_UNK   0x8410     // cinza

static constexpr auto LGFX_FONT = &fonts::Font2;

// Thresholds de latência (ms)
static constexpr uint16_t LAT_WARN_MS = 1200;
static constexpr uint16_t LAT_SLOW_MS = 3000;

// Util cor
static inline uint16_t RGB(uint8_t r,uint8_t g,uint8_t b){
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
