#include "touch.hpp"
#include "config.hpp"
#include "ui.hpp"                   // <-- necessário p/ UI::SW(), UI::SH()
#include <SPI.h>
#include <XPT2046_Touchscreen.h>

static SPIClass hspi(HSPI);
static XPT2046_Touchscreen tp(T_CS, T_IRQ);

namespace Touch {

void beginHSPI(){
  pinMode(T_CS, OUTPUT);  digitalWrite(T_CS, HIGH);
  pinMode(T_IRQ, INPUT);
  hspi.end();
  hspi.begin(T_SCK, T_MISO, T_MOSI, T_CS);
  tp.begin(hspi);
  tp.setRotation(0);   // mapeamento manual (MAP4)
}

bool touched(){
  return digitalRead(T_IRQ) == LOW;
}

void readRaw(int16_t& rx,int16_t& ry,int16_t& rz){
  auto p = tp.getPoint();
  rx = p.x; ry = p.y; rz = p.z;
}

// MAP4: SWAP_XY cru (validado)
void mapRawToScreen(int16_t rx,int16_t ry,int& sx,int& sy){
  int16_t xraw = ry;  // swap
  int16_t yraw = rx;  // swap
  long nx = map(xraw, RAW_Y_MIN, RAW_Y_MAX, 0, UI::SW()-1);
  long ny = map(yraw, RAW_X_MIN, RAW_X_MAX, 0, UI::SH()-1);
  sx = constrain((int)nx, 0, UI::SW()-1);
  sy = constrain((int)ny, 0, UI::SH()-1);
}

} // namespace Touch
