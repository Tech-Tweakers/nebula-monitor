#pragma once
#include <LovyanGFX.hpp>
#include "config.hpp"

enum Status : uint8_t { UNKNOWN=0, UP=1, DOWN=2 };

struct TileInfo {
  const char* name;
  Status st;
  uint16_t lat_ms;
};

namespace UI {
  void     begin(uint8_t rot = ROT, int bl_pin = BL_PIN);
  int      SW();
  int      SH();
  int      barHeight();
  void     setCols(int cols);
  void     computeGrid(int nTargets);
  void     safeWipeAll(uint8_t finalRot = ROT);

  // Top bar (duas assinaturas)
  void     drawTopBar(bool autoRefresh, uint32_t lastRefresh_ms);
  void     drawTopBar(bool autoRefresh, uint32_t lastRefresh_ms, const char* rightHint);

  void     showFooter(const char* msg);

  void     drawTile(int idx, const TileInfo& t);
  void     drawAllTiles(const TileInfo* arr, int n);
  bool     insideTile(int idx, int sx, int sy);
}
