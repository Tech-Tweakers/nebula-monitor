#include "touch.hpp"
#include "config.hpp"
#include "ui.hpp"
#include <SPI.h>
#include <XPT2046_Touchscreen.h>

static SPIClass hspi(HSPI);
static XPT2046_Touchscreen tp(T_CS, T_IRQ);

void Touch::beginHSPI(){
  pinMode(T_CS, OUTPUT);  digitalWrite(T_CS, HIGH);
  pinMode(T_IRQ, INPUT);
  hspi.end();
  hspi.begin(T_SCK, T_MISO, T_MOSI, T_CS);
  tp.begin(hspi);
  tp.setRotation(0); // mapeamos manualmente (SWAP_XY)
}

bool Touch::touched(){
  return digitalRead(T_IRQ) == LOW;
}

void Touch::readRaw(int16_t& rx, int16_t& ry, int16_t& rz){
  auto p = tp.getPoint();
  rx = p.x; ry = p.y; rz = p.z;
}

// MAP 4: SWAP_XY e ranges trocados
void Touch::mapRawToScreen(int16_t rx, int16_t ry, int& sx, int& sy){
  int16_t xraw = ry;
  int16_t yraw = rx;

  long nx = map(xraw, RAW_Y_MIN, RAW_Y_MAX, 0, UI::SW()-1);
  long ny = map(yraw, RAW_X_MIN, RAW_X_MAX, 0, UI::SH()-1);

  if (nx < 0) nx = 0; if (nx > UI::SW()-1) nx = UI::SW()-1;
  if (ny < 0) ny = 0; if (ny > UI::SH()-1) ny = UI::SH()-1;

  sx = (int)nx;
  sy = (int)ny;
}
