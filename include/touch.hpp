#pragma once
#include <Arduino.h>

namespace Touch {
  void beginHSPI();                            // inicia o XPT no HSPI compartilhado
  bool touched();                              // TRUE se IRQ baixo
  void readRaw(int16_t& rx, int16_t& ry, int16_t& rz);  // getPoint()
  void mapRawToScreen(int16_t rx, int16_t ry, int& sx, int& sy); // SWAP_XY fixo
}
