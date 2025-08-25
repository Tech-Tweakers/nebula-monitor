#include <Arduino.h>

#define LGFX_USE_V1
#define LGFX_AUTODETECT
#include <LovyanGFX.hpp>

// (algumas instalações precisam deste header; se não existir, ignore)
#if __has_include(<lgfx_user/LGFX_AUTODETECT.hpp>)
  #include <lgfx_user/LGFX_AUTODETECT.hpp>
#elif __has_include(<LGFX_AUTODETECT.hpp>)
  #include <LGFX_AUTODETECT.hpp>
#endif

#include <XPT2046_Touchscreen.h>

// ---------- Display (Lovyan autodetect) ----------
static LGFX display;
static constexpr auto LGFX_FONT = &fonts::Font0;

// ---------- Touch XPT2046 (VSPI dedicado) ----------
#define XPT2046_IRQ  36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK  25
#define XPT2046_CS   33
SPIClass spiTouch(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

// ---------- Backlight ----------
const int BL_PIN = 27;

// ---------- Dimensões / rotação ----------
int16_t SW = 240, SH = 320;
const uint8_t ROT = 2;  // troque se quiser testar outras rotações

// ---------- Calibração bruta do touch (ajuste fino se precisar) ----------
const int RAW_X_MIN = 200,  RAW_X_MAX = 3700;
const int RAW_Y_MIN = 240,  RAW_Y_MAX = 3800;

// ---------- Cores 565 ----------
static inline uint16_t RGB(uint8_t r,uint8_t g,uint8_t b){return ((r & 0xF8) << 8)|((g & 0xFC) << 3)|(b >> 3);}
const uint16_t PALETTE[] = {
  RGB(220,60,60),  RGB(255,160,0), RGB(255,220,0), RGB(0,200,90),
  RGB(0,180,200),  RGB(70,100,255), RGB(180,80,220), RGB(240,240,240)
};
const int N_COL = sizeof(PALETTE)/sizeof(PALETTE[0]);

// ---------- Retângulos de canto ----------
struct Rect { int x,y,w,h; int colorIdx; };
Rect rects[4];

bool touching=false;
const uint16_t DEBOUNCE_MS = 120;
uint32_t lastTapMs = 0;

void computeRects(){
  // tamanho relativo pra caber em qualquer resolução
  int margin = 6;
  int rw = max(60, SW/4);  // largura mínima 60
  int rh = max(40, SH/6);  // altura mínima 40

  rects[0] = { margin,                 margin,                 rw, rh, 0 }; // TL
  rects[1] = { SW - margin - rw,       margin,                 rw, rh, 1 }; // TR
  rects[2] = { margin,                 SH - margin - rh,       rw, rh, 2 }; // BL
  rects[3] = { SW - margin - rw,       SH - margin - rh,       rw, rh, 3 }; // BR
}

void drawRectBox(int i){
  uint16_t c = PALETTE[ rects[i].colorIdx % N_COL ];
  display.fillRoundRect(rects[i].x, rects[i].y, rects[i].w, rects[i].h, 8, c);
  // número no centro
  display.setFont(LGFX_FONT);
  display.setTextColor(RGB(20,20,20), c);
  char num[4]; snprintf(num,sizeof(num), "%d", i+1);
  int16_t tw = display.textWidth(num), th = display.fontHeight();
  int cx = rects[i].x + rects[i].w/2 - tw/2;
  int cy = rects[i].y + rects[i].h/2 - th/2;
  display.setCursor(cx, cy);
  display.print(num);
}

void drawAll(){
  display.fillScreen(TFT_BLACK);
  for(int i=0;i<4;i++) drawRectBox(i);

  // overlay com WxH/rot pra debug
  display.setFont(LGFX_FONT);
  display.setTextColor(TFT_WHITE, TFT_BLACK);
  char buf[32]; snprintf(buf,sizeof(buf), "%dx%d  rot=%u", SW, SH, ROT);
  int16_t tw = display.textWidth(buf), th = display.fontHeight();
  display.setCursor(SW/2 - tw/2, 2);
  display.print(buf);
}

bool inside(int x,int y,const Rect& r){
  return x>=r.x && x<=r.x+r.w && y>=r.y && y<=r.y+r.h;
}

// mapeia raw -> pixels; mantém espelho usado no teu setup pra rot=2
void mapTouch(const TS_Point& p, int& sx, int& sy){
  sx = map(p.x, RAW_X_MIN, RAW_X_MAX, 1, SW);
  sy = map(p.y, RAW_Y_MIN, RAW_Y_MAX, 1, SH);

  // ajuste por rotação (a lib faz parte, mas mantemos este espelho
  // pq teu módulo pediu isso no ROT=2 durante os testes anteriores)
  if (ROT == 2){
    sx = SW - sx;
    sy = SH - sy;
  }
}

void setup(){
  Serial.begin(115200);
  delay(60);

  // Display (autodetect) + backlight manual
  display.begin();
  pinMode(BL_PIN, OUTPUT);
  digitalWrite(BL_PIN, HIGH);      // liga BL (se seu BL for ativo baixo, troque por LOW)
  display.setColorDepth(16);
  display.setRotation(ROT);

  SW = display.width(); SH = display.height();
  computeRects();
  for(int i=0;i<4;i++) rects[i].colorIdx = i; // cores iniciais
  drawAll();

  // Touch em VSPI
  spiTouch.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(spiTouch);
  touchscreen.setRotation(ROT);

  Serial.printf("Lovyan autodetect: %dx%d rot=%u\n", SW, SH, ROT);
}

void loop(){
  bool nowTouch = touchscreen.touched();
  if (nowTouch && !touching){
    TS_Point p = touchscreen.getPoint();
    int sx, sy; mapTouch(p, sx, sy);
    Serial.printf("RAW x=%d y=%d z=%d  ->  %d,%d\n", p.x, p.y, p.z, sx, sy);

    uint32_t now = millis();
    if (now - lastTapMs > DEBOUNCE_MS){
      for(int i=0;i<4;i++){
        if (inside(sx, sy, rects[i])){
          rects[i].colorIdx = (rects[i].colorIdx + 1) % N_COL;
          drawRectBox(i);
          break;
        }
      }
      lastTapMs = now;
    }
  }
  touching = nowTouch;
}
