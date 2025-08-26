#include "ui.hpp"
#include <algorithm>
#include <FS.h>
#include <LittleFS.h>
using std::max;

// Helpers locais
static inline uint16_t RGB(uint8_t r,uint8_t g,uint8_t b){
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// Estado do display/layout
static LGFX display;
static int16_t gSW = 240, gSH = 320;
static int gBAR_H = 30, gFOOTER_H = 16;
static const int PAD = 8, R = 6;
static int COLS = 2;
static int rows = 0, tileW = 0, tileH = 0, gridTop = 0;

// --- coloque isso perto do topo de src/ui.cpp (depois dos includes/vars) ---
static inline void printBold(int16_t x, int16_t y, const char* s, uint16_t fg, uint16_t bg){
  display.setTextColor(fg, bg);
  display.setCursor(x-1, y); display.print(s);
  display.setCursor(x+1, y); display.print(s);
  display.setCursor(x, y-1); display.print(s);
  display.setCursor(x, y+1); display.print(s);
  display.setCursor(x, y);   display.print(s);
}

namespace UI {

void begin(uint8_t rot, int bl_pin){
  display.begin();
  pinMode(bl_pin, OUTPUT);
  digitalWrite(bl_pin, HIGH);
  display.setColorDepth(16);
  display.setRotation(rot);
  display.setFont(LGFX_FONT);
  gSW = display.width();
  gSH = display.height();
}

void safeWipeAll(uint8_t finalRot){
  display.setClipRect(0,0,display.width(),display.height());
  for (uint8_t r=0;r<4;r++){ display.setRotation(r); display.fillScreen(COL_BG); delay(2); }
  display.setRotation(finalRot);
  display.fillScreen(COL_BG);
}

int SW(){ return gSW; }
int SH(){ return gSH; }
int barHeight(){ return gBAR_H; }
void setCols(int c){ COLS = (c<1?1:c); }

void computeGrid(int nTargets){
  rows   = (nTargets + COLS - 1)/COLS;
  gridTop= gBAR_H + PAD;
  tileW  = (gSW - (COLS + 1)*PAD) / COLS;
  int availH = gSH - gridTop - (rows + 1)*PAD - gFOOTER_H;
  tileH  = std::max(34, availH / rows);
}

static void tilePos(int idx, int& x, int& y){
  int r = idx / COLS;
  int c = idx % COLS;
  x = PAD + c*(tileW + PAD);
  y = gridTop + PAD + r*(tileH + PAD);
}

void drawTopBar(bool autoRefresh, uint32_t lastRefresh)
{
  const uint16_t BAR_BG = COL_BAR;  // fundo original (ex.: TFT_DARKGREY)
  const uint16_t BAR_FG = COL_TXT;  // texto branco

  display.fillRect(0, 0, gSW, gBAR_H, BAR_BG);
  display.setFont(LGFX_FONT);

  // título central
  const char* title = "Nebula Remote Monitoring";
  int16_t tw = display.textWidth(title);
  int16_t th = display.fontHeight();
  int16_t ty = (gBAR_H - th)/2;
  printBold((gSW - tw)/2, ty, title, BAR_FG, BAR_BG);

  // “Auto: …” à esquerda
  // char left[24];
  // snprintf(left, sizeof(left), "Auto: %s", autoRefresh ? "ON" : "OFF");
  // printBold(8, ty, left, BAR_FG, BAR_BG);

  // carimbo de tempo à direita
  // char right[24];
  // snprintf(right, sizeof(right), "t=%lus", (unsigned long)(lastRefresh/1000));
  // int16_t twR = display.textWidth(right);
  // printBold(gSW - 8 - twR, ty, right, BAR_FG, BAR_BG);
}

void showFooter(const char* msg){
  display.fillRect(0, gSH - gFOOTER_H, gSW, gFOOTER_H, COL_BG);
  display.setFont(LGFX_FONT);
  display.setTextColor(COL_TXT, COL_BG);
  int16_t tw = display.textWidth(msg);
  display.setCursor((gSW - tw)/2, gSH - gFOOTER_H + (gFOOTER_H - display.fontHeight())/2);
  display.print(msg);
}

void drawTile(int idx, const TileInfo& t){
  int x,y; tilePos(idx,x,y);

  uint16_t bg = (t.st==UP) ? COL_UP : (t.st==DOWN ? COL_DOWN : COL_UNK);

  // antes: uint16_t fg = (t.st==UP) ? TFT_BLACK : TFT_WHITE;
  uint16_t fg = TFT_WHITE;     // <- sempre branco (fica ótimo no verde escuro)

  display.fillRoundRect(x, y, tileW, tileH, R, bg);
  display.setFont(LGFX_FONT);
  display.setTextColor(fg, bg);

  // nome
  display.setCursor(x+6, y+4);
  display.print(t.name);

  // status + latência
  char buf[40];
  const char* sname = (t.st==UP) ? "UP" : (t.st==DOWN ? "DOWN" : "UNK");
  if (t.st==UP) snprintf(buf,sizeof(buf), "%s %ums", sname, (unsigned)t.lat_ms);
  else          snprintf(buf,sizeof(buf), "%s", sname);
  display.setCursor(x+6, y+4 + display.fontHeight());
  display.print(buf);
}

bool insideTile(int idx, int sx, int sy){
  int x,y; tilePos(idx,x,y);
  return sx>=x && sx<=x+tileW && sy>=y && sy<=y+tileH;
}

// --------- splash com PNG no LittleFS ----------
void splashLogo(const char* pngPath,const char* title,const char* subtitle,uint16_t hold_ms){
  display.fillScreen(COL_BG);

  int cw = gSW - 2*PAD; if (cw > 220) cw = 220;
  int ch = gSH/2;       if (ch > 160) ch = 160;
  int cx = (gSW - cw)/2;
  int cy = (gSH - ch)/2;

  display.fillRoundRect(cx, cy, cw, ch, 10, TFT_BLACK);
  display.drawRoundRect(cx, cy, cw, ch, 10, TFT_DARKGREY);

  bool pngShown = false;
  if (LittleFS.begin(false) || LittleFS.begin(true)) {
    if (pngPath && LittleFS.exists(pngPath)) {
      File f = LittleFS.open(pngPath, "r");
      if (f) {
        // centraliza logo (ajuste a largura/altura do seu PNG)
        int px = cx + (cw - 120)/2;
        int py = cy + 10;
        if (display.drawPng(&f, px, py)) pngShown = true;  // <- ponteiro!
        f.close();
      }
    }
  }

  display.setFont(LGFX_FONT);
  display.setTextColor(COL_TXT, TFT_BLACK);
  display.setTextWrap(false);

  int yText = cy + (pngShown ? 10 + 64 + 6 : 18);
  if (title && *title) {
    int16_t tw = display.textWidth(title);
    display.setCursor(max(0, cx + (cw - tw)/2), yText);
    display.print(title);
    yText += display.fontHeight() + 4;
  }
  if (subtitle && *subtitle) {
    int16_t tw = display.textWidth(subtitle);
    display.setCursor(max(0, cx + (cw - tw)/2), yText);
    display.print(subtitle);
  }

  // Progress bar
  int pw = cw - 40; if (pw < 60) pw = cw - 20;
  int ph = 6;
  int px = cx + (cw - pw)/2;
  int py = cy + ch - ph - 14;
  display.drawRoundRect(px, py, pw, ph, 3, TFT_DARKGREY);

  uint32_t t0 = millis();
  while (millis() - t0 < hold_ms) {
    uint32_t dt = millis() - t0;
    int fillw = (int)((pw-2) * (float)dt / (float)hold_ms);
    if (fillw < 0) fillw = 0;
    if (fillw > (pw-2)) fillw = pw-2;
    display.fillRoundRect(px+1, py+1, fillw, ph-2, 2, TFT_DARKGREY);
    delay(18);
  }
}

} // namespace UI
