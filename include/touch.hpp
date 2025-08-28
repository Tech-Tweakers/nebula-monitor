#pragma once
#include <Arduino.h>

class Touch {
public:
  static bool beginHSPI();
  static bool touched();
  static void readRaw(int16_t& x, int16_t& y, int16_t& z);
  static void mapRawToScreen(int16_t rx, int16_t ry, int& sx, int& sy);
};
