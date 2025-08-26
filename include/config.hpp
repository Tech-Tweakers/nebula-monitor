#pragma once

// Evita dups dos defines se também vierem via build_flags
#ifndef LGFX_USE_V1
  #define LGFX_USE_V1
#endif
#ifndef LGFX_AUTODETECT
  #define LGFX_AUTODETECT
#endif

#include <LovyanGFX.hpp>

// ----------------- Hardware base -----------------
static constexpr uint8_t ROT    = 2;   // rotação da tua montagem
static constexpr int     BL_PIN = 27;  // backlight

// Touch (XPT2046) no HSPI compartilhado com o LCD
static constexpr int T_SCK  = 14;
static constexpr int T_MOSI = 13;
static constexpr int T_MISO = 12;
static constexpr int T_CS   = 33;
static constexpr int T_IRQ  = 36; // ativo em LOW

// Calibração bruta (a gente mapeia com SWAP_XY)
static constexpr int RAW_X_MIN = 200;
static constexpr int RAW_X_MAX = 3700;
static constexpr int RAW_Y_MIN = 240;
static constexpr int RAW_Y_MAX = 3800;

// Fonte padrão de UI
static constexpr auto LGFX_FONT = &fonts::Font2;

// Cores base
#define COL_BG    TFT_BLACK
#define COL_BAR   TFT_DARKGREY
#define COL_TXT   TFT_WHITE
// antigo: #define COL_UP  0x07E0   // verde puro, muito “aceso”
// novo (verde escuro, bom contraste c/ texto preto):
#define COL_UP    RGB(0,140,60)
// mantém
#define COL_DOWN  0xF800
#define COL_UNK   0x8410

// Estados (compartilhado entre main/ui)
enum Status : uint8_t { UNKNOWN=0, UP=1, DOWN=2 };

// Info que desenhamos em cada tile
struct TileInfo {
  const char* name;
  Status      st;
  uint16_t    lat_ms;
};
