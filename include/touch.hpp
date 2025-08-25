#pragma once
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include "config.hpp"

namespace Touch {
  void beginHSPI(int sck=T_SCK, int miso=T_MISO, int mosi=T_MOSI, int cs=T_CS, int irq=T_IRQ);
  bool irqActive();                               // LOW = toque
  bool readRaw(int16_t& rx,int16_t& ry,int16_t& rz);
  void mapRawToScreen(int16_t rx,int16_t ry,int& sx,int& sy, int SW, int SH); // MAP 4 (swapXY)
}
