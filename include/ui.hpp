#pragma once
#include <LovyanGFX.hpp>
#include "config.hpp"

namespace UI {
  // Init / dimensões / layout
  void     begin(uint8_t rot = ROT, int bl_pin = BL_PIN);
  void     safeWipeAll(uint8_t finalRot = ROT);
  int      SW();
  int      SH();
  int      barHeight();
  void     setCols(int cols);
  void     computeGrid(int nTargets);

  // Desenho de topo/rodapé
  void     drawTopBar(bool autoRefresh, uint32_t lastRefresh_ms);
  void     showFooter(const char* msg);

  // Tiles
  void     drawTile(int idx, const TileInfo& t);
  bool     insideTile(int idx, int sx, int sy);

  // (Opcional) Splash com PNG no LittleFS (fallback se não houver)
  void     splashLogo(const char* pngPath = "/tt_logo.png",
                      const char* title   = "Tech-Tweakers",
                      const char* subtitle= "Nebula Monitor v0.1.0",
                      uint16_t hold_ms    = 900);
}
