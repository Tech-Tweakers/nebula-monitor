#include "touch.hpp"

static SPIClass hspi(HSPI);
static XPT2046_Touchscreen tp(T_CS, T_IRQ);

namespace Touch {

void beginHSPI(int sck, int miso, int mosi, int cs, int irq){
  pinMode(cs, OUTPUT);  digitalWrite(cs, HIGH);
  pinMode(irq, INPUT);
  hspi.end();
  hspi.begin(sck, miso, mosi, cs);
  tp = XPT2046_Touchscreen(cs, irq);
  tp.begin(hspi);
  tp.setRotation(0);      // mapeamos manualmente
}

bool irqActive(){ return digitalRead(T_IRQ) == LOW; }

bool readRaw(int16_t& rx,int16_t& ry,int16_t& rz){
  if (!irqActive()) return false;
  TS_Point p = tp.getPoint();
  rx = p.x; ry = p.y; rz = p.z;
  return true;
}

// MAP 4 (swapXY), sem flips
void mapRawToScreen(int16_t rx,int16_t ry,int& sx,int& sy, int SW, int SH){
  // valores brutos conforme teu range
  static const int RAW_X_MIN = 200,  RAW_X_MAX = 3700;
  static const int RAW_Y_MIN = 240,  RAW_Y_MAX = 3800;

  // swap cru
  int16_t xraw = ry;
  int16_t yraw = rx;

  long nx = map(xraw, RAW_Y_MIN, RAW_Y_MAX, 0, SW-1);
  long ny = map(yraw, RAW_X_MIN, RAW_X_MAX, 0, SH-1);
  sx = constrain((int)nx, 0, SW-1);
  sy = constrain((int)ny, 0, SH-1);
}

} // namespace Touch
