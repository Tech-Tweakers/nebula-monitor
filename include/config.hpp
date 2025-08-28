#pragma once

#include <TFT_eSPI.h>
#include <SPI.h>

// ----------------- Hardware base -----------------
static constexpr uint8_t ROT    = 2;   // rotação landscape (como no tutorial)
static constexpr int     BL_PIN = 27;  // backlight

// Touch (XPT2046) - pinagem CORRETA da CYD
static constexpr int T_SCK  = 14;  // T_CLK (HSPI)
static constexpr int T_MOSI = 13;  // T_DIN (HSPI)
static constexpr int T_MISO = 12;  // T_OUT (HSPI)
static constexpr int T_CS   = 33;  // T_CS
static constexpr int T_IRQ  = 36;  // T_IRQ

// Calibração da CYD (do tutorial)
static constexpr int RAW_X_MIN = 200;
static constexpr int RAW_X_MAX = 3700;
static constexpr int RAW_Y_MIN = 240;
static constexpr int RAW_Y_MAX = 3800;

// RGB color conversion macro
#define RGB(r,g,b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))

// Cores base (usando TFT_eSPI)
#define COL_BG    TFT_BLACK
#define COL_BAR   TFT_DARKGREY
#define COL_TXT   TFT_WHITE
#define COL_UP    RGB(0,140,60)
#define COL_DOWN  TFT_RED
#define COL_UNK   TFT_DARKGREY

// Estados (compartilhado entre main/ui)
enum Status : uint8_t { UNKNOWN=0, UP=1, DOWN=2 };

// Target struct para network scanning
struct Target {
  const char* name;
  const char* url;
  Status  st;
  uint16_t lat_ms;
};

// Info que desenhamos em cada tile
struct TileInfo {
  const char* name;
  Status      st;
  uint16_t    lat_ms;
};
