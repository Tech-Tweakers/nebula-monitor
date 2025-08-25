#include <Arduino.h>

#define LGFX_USE_V1
#define LGFX_AUTODETECT
#include <LovyanGFX.hpp>
#if __has_include(<lgfx_user/LGFX_AUTODETECT.hpp>)
  #include <lgfx_user/LGFX_AUTODETECT.hpp>
#elif __has_include(<LGFX_AUTODETECT.hpp>)
  #include <LGFX_AUTODETECT.hpp>
#endif

#include <SPI.h>
#include <XPT2046_Touchscreen.h>

// ---------------- Display (Lovyan autodetect) ----------------
static LGFX display;
static constexpr auto LGFX_FONT = &fonts::Font0;

static const int BL_PIN = 27;
static const uint8_t ROT = 2;
int16_t SW = 240, SH = 320;

// ---------------- Touch XPT2046 (HSPI compartilhado com LCD) ----------------
// Muitos módulos 2.8" ligam o XPT no MESMO bus do TFT (HSPI: 14/13/12)
static const int T_SCK  = 14;   // HSPI SCK (mesmo do TFT)
static const int T_MOSI = 13;   // HSPI MOSI (mesmo do TFT)
static const int T_MISO = 12;   // HSPI MISO (mesmo do TFT)
static const int T_IRQ  = 36;   // IRQ ativo em LOW

// CS candidato (alguns usam 33, outros 26)
static int T_CS = 33;

SPIClass hspi(HSPI);
XPT2046_Touchscreen* tp = nullptr;

// Calibração bruta (afinamos depois)
static int RAW_X_MIN = 200,  RAW_X_MAX = 3700;
static int RAW_Y_MIN = 240,  RAW_Y_MAX = 3800;

// ---------------- UI: 4 cantos ----------------
static inline uint16_t RGB(uint8_t r,uint8_t g,uint8_t b){
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
const uint16_t PALETTE[] = {
  RGB(220,60,60), RGB(255,160,0), RGB(255,220,0), RGB(0,200,90),
  RGB(0,180,200), RGB(70,100,255), RGB(180,80,220), RGB(240,240,240)
};
const int N_COL = sizeof(PALETTE)/sizeof(PALETTE[0]);

struct Rect { int x,y,w,h; int colorIdx; };
Rect rects[4];
bool touching=false;
const uint16_t DEBOUNCE_MS = 120;
uint32_t lastTapMs = 0;

// ---------------- Helpers ----------------
void computeRects(){
  int margin = 6;
  int rw = max(60, SW/4);
  int rh = max(40, SH/6);
  rects[0] = { margin,           margin,           rw, rh, 0 }; // TL
  rects[1] = { SW - margin - rw, margin,           rw, rh, 1 }; // TR
  rects[2] = { margin,           SH - margin - rh, rw, rh, 2 }; // BL
  rects[3] = { SW - margin - rw, SH - margin - rh, rw, rh, 3 }; // BR
}
void drawRectBox(int i){
  uint16_t c = PALETTE[ rects[i].colorIdx % N_COL ];
  display.fillRoundRect(rects[i].x, rects[i].y, rects[i].w, rects[i].h, 8, c);
  display.setFont(LGFX_FONT);
  display.setTextColor(RGB(20,20,20), c);
  char num[4]; snprintf(num,sizeof(num), "%d", i+1);
  int16_t tw = display.textWidth(num), th = display.fontHeight();
  int cx = rects[i].x + rects[i].w/2 - tw/2;
  int cy = rects[i].y + rects[i].h/2 - th/2;
  display.setCursor(cx, cy); display.print(num);
}
void drawAll(){
  display.fillScreen(TFT_BLACK);
  for(int i=0;i<4;i++) drawRectBox(i);
  display.setFont(LGFX_FONT);
  display.setTextColor(TFT_WHITE, TFT_BLACK);
  char buf[48]; snprintf(buf,sizeof(buf), "%dx%d rot=%u  (CS=%d, HSPI 14/13/12, IRQ=36)", SW, SH, ROT, T_CS);
  int16_t tw = display.textWidth(buf);
  display.setCursor(SW/2 - tw/2, 2); display.print(buf);
}
bool inside(int x,int y,const Rect& r){ return x>=r.x && x<=r.x+r.w && y>=r.y && y<=r.y+r.h; }

// mapeia raw -> pixels e aplica espelho do ROT=2 (como vimos no teu)
void mapTouchRawToScreen(int16_t rx,int16_t ry,int& sx,int& sy){
  sx = map(rx, RAW_X_MIN, RAW_X_MAX, 1, SW);
  sy = map(ry, RAW_Y_MIN, RAW_Y_MAX, 1, SH);
  if (ROT == 2){ sx = SW - sx; sy = SH - sy; }
}

// limpa “fantasmas” sem resetar registro do painel
void safeWipeAll(uint8_t finalRot = ROT){
  display.setClipRect(0, 0, display.width(), display.height());
  for (uint8_t r = 0; r < 4; r++){
    display.setRotation(r);
    display.fillScreen(TFT_BLACK);
    delay(2);
  }
  display.setRotation(finalRot);
  display.fillScreen(TFT_BLACK);
}

// ---------------- Touch init (HSPI, tenta CS=33 e depois CS=26) ----------------
bool initTouchHSPI(int cs){
  // Compartilhando o bus com o LCD: SCK/MOSI/MISO fixos (14/13/12)
  hspi.end();
  pinMode(cs, OUTPUT);  digitalWrite(cs, HIGH);
  pinMode(T_IRQ, INPUT);   // GPIO36 não tem pull-up interno; módulo costuma ter
  hspi.begin(T_SCK, T_MISO, T_MOSI, cs);

  if (tp) { delete tp; tp = nullptr; }
  tp = new XPT2046_Touchscreen(cs, T_IRQ);
  if (!tp->begin(hspi)) return false;
  tp->setRotation(0);   // manteremos o mapping manual (espelho no ROT)

  // Sanity check curto: só aceitaremos se IRQ cair e X/Y variarem
  int hits=0, moves=0; int16_t lastx=-1,lasty=-1;
  uint32_t t0 = millis();
  while (millis() - t0 < 350){
    if (digitalRead(T_IRQ) == LOW){
      TS_Point p = tp->getPoint();
      if (p.z > 5){
        hits++;
        if (p.x != lastx || p.y != lasty) moves++;
        lastx = p.x; lasty = p.y;
      }
    }
    delay(20);
  }
  return (hits >= 2) && (moves >= 1);
}

// ---------------- Setup ----------------
void setup(){
  Serial.begin(115200);
  delay(80);

  // LCD
  display.begin();
  pinMode(BL_PIN, OUTPUT); digitalWrite(BL_PIN, HIGH);
  display.setColorDepth(16);
  display.setRotation(ROT);
  SW = display.width(); SH = display.height();
  safeWipeAll(ROT);

  computeRects(); for(int i=0;i<4;i++) rects[i].colorIdx = i;
  drawAll();

  // TOUCH: tenta CS=33, se falhar tenta CS=26
  Serial.println("\n[Touch] Inicializando em HSPI (compartilhado com o LCD)...");
  if (!initTouchHSPI(33)){
    Serial.println("[Touch] CS=33 falhou, tentando CS=26...");
    if (initTouchHSPI(26)){
      T_CS = 26;
    } else {
      Serial.println("[Touch] Nenhum CS respondeu. Confira CS e DOUT (MISO) do touch.");
    }
  } else {
    T_CS = 33;
  }

  Serial.printf("[Touch] HSPI SCK=%d MOSI=%d MISO=%d CS=%d IRQ=%d\n",
                T_SCK, T_MOSI, T_MISO, T_CS, T_IRQ);
}

// ---------------- Loop ----------------
void loop(){
  if (!tp) return;

  // Gate por IRQ real: só lê quando LOW
  bool nowTouch = (digitalRead(T_IRQ) == LOW);
  if (nowTouch && !touching){
    TS_Point p = tp->getPoint();
    Serial.printf("RAW x=%d y=%d z=%d\n", p.x, p.y, p.z);

    int sx, sy; mapTouchRawToScreen(p.x, p.y, sx, sy);

    uint32_t now = millis();
    if (now - lastTapMs > DEBOUNCE_MS){
      for (int i=0;i<4;i++){
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
