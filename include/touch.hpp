#pragma once
#include <Arduino.h>

namespace Touch {
  void beginHSPI();                            // inicializa XPT2046 em HSPI
  bool touched();                              // IRQ gate (LOW = toque)
  void readRaw(int16_t& rx,int16_t& ry,int16_t& rz);
  void mapRawToScreen(int16_t rx,int16_t ry,int& sx,int& sy); // MAP4 (swap XY)
}
