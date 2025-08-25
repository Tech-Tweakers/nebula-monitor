#pragma once

// Defina apenas se não vierem do build (-D ...)
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

// ——— PINOS / HARDWARE ———
static constexpr uint8_t ROT    = 2;
static constexpr int     BL_PIN = 27;

// Touch (HSPI compartilhado com o LCD)
static constexpr int T_SCK  = 14;  // HSPI SCK
static constexpr int T_MOSI = 13;  // HSPI MOSI
static constexpr int T_MISO = 12;  // HSPI MISO
static constexpr int T_CS   = 33;  // Touch CS
static constexpr int T_IRQ  = 36;  // Touch IRQ (LOW = toque)

// Wi-Fi
static constexpr const char* WIFI_SSID = "Polaris";
static constexpr const char* WIFI_PASS = "55548502";

// Cores
#define COL_BG    TFT_BLACK
#define COL_BAR   TFT_DARKGREY
#define COL_TXT   TFT_WHITE
#define COL_UP    0x07E0
#define COL_WARN  0xFFE0
#define COL_SLOW  0xFD20
#define COL_DOWN  0xF800
#define COL_UNK   0x8410

// Fonte padrão
static constexpr auto LGFX_FONT = &fonts::Font2;

// Limiares de cor (ms)
static constexpr uint16_t LAT_WARN_MS = 1200;  // verde -> amarelo
static constexpr uint16_t LAT_SLOW_MS = 3000;  // amarelo -> laranja

// Util cor
static inline uint16_t RGB(uint8_t r,uint8_t g,uint8_t b){
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
