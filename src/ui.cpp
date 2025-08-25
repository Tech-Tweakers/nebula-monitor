#include "ui.hpp"
#include <algorithm>
using std::max;

static LGFX display;
static int16_t gSW=240, gSH=320;
static int gBAR_H=30, gFOOTER_H=16;
static const int PAD=8, R=6;
static int COLS=2;
static int rows=0, tileW=0, tileH=0, gridTop=0;

namespace UI {

void begin(uint8_t rot, int bl_pin){
  display.begin();
  pinMode(bl_pin, OUTPUT); digitalWrite(bl_pin, HIGH);
  display.setColorDepth(16);
  display.setRotation(rot);
  display.setTextWrap(false);
  display.setFont(LGFX_FONT);

  gSW = display.width();
  gSH = display.height();
  gBAR_H    = display.fontHeight() + 8;
  gFOOTER_H = display.fontHeight() + 4;
}

int SW(){ return gSW; }
int SH(){ return gSH; }
int barHeight(){ return gBAR_H; }
void setCols(int cols){ COLS = (cols<1)?1:cols; }

void computeGrid(int nTargets){
  rows    = (nTargets + COLS - 1) / COLS;
  gridTop = gBAR_H + PAD;
  tileW   = (gSW - (COLS + 1)*PAD) / COLS;
  int availH = gSH - gridTop - (rows + 1)*PAD - gFOOTER_H;
  int needH  = 2*display.fontHeight() + 10;
  tileH = max(needH, availH / rows);
}

// Overload antigo (2 args): encaminha para a versão nova (3 args)
void drawTopBar(bool autoRefresh, uint32_t lastRefresh_ms){
  drawTopBar(autoRefresh, lastRefresh_ms, nullptr);
}

// Versão nova (3 args) com hint à direita
void drawTopBar(bool autoRefresh, uint32_t lastRefresh_ms, const char* rightHint){
  display.fillRect(0,0,gSW,gBAR_H, COL_BAR);
  display.setFont(LGFX_FONT);
  display.setTextColor(COL_TXT, COL_BAR);
  display.setTextWrap(false);

    const char* title = "Nebula Monitoring v0.1.0";
    int16_t y = (gBAR_H - display.fontHeight())/2;
    display.setCursor(PAD, y);
    display.print(title);

  char right[32];
  if (rightHint && *rightHint) {
    snprintf(right, sizeof(right), "%s", rightHint);
  } else {
    snprintf(right, sizeof(right), "t=%lus", (unsigned long)(lastRefresh_ms/1000));
  }
  int16_t tw2 = display.textWidth(right);
  display.setCursor(gSW - PAD - tw2, y); display.print(right);
}

void showFooter(const char* msg){
  display.fillRect(0, gSH - gFOOTER_H, gSW, gFOOTER_H, COL_BG);
  display.setFont(LGFX_FONT);
  display.setTextColor(COL_TXT, COL_BG);
  display.setTextWrap(false);
  int16_t tw = display.textWidth(msg);
  int16_t y  = gSH - gFOOTER_H + (gFOOTER_H - display.fontHeight())/2;
  display.setCursor(std::max(0, (gSW - tw)/2), y);
  display.print(msg);
}

static String fitEllipsis(const char* s, int maxW){
  String out = s;
  if (display.textWidth(out.c_str()) <= maxW) return out;
  while (out.length() && display.textWidth((out + "...").c_str()) > maxW) {
    out.remove(out.length()-1);
  }
  if (out.length()) out += "...";
  return out;
}

static void tilePos(int idx, int& x, int& y){
  int r = idx / COLS, c = idx % COLS;
  x = PAD + c*(tileW + PAD);
  y = gridTop + PAD + r*(tileH + PAD);
}

static uint16_t colorFor(const TileInfo& t){
  if (t.st == DOWN)    return COL_DOWN;
  if (t.st == UNKNOWN) return COL_UNK;
  if (t.lat_ms <= LAT_WARN_MS) return COL_UP;
  if (t.lat_ms <= LAT_SLOW_MS) return COL_WARN;
  return COL_SLOW;
}

void drawTile(int idx, const TileInfo& t){
  int x,y; tilePos(idx,x,y);
  uint16_t bg = colorFor(t);
  uint16_t fg = (bg==COL_UP || bg==COL_WARN) ? TFT_BLACK : TFT_WHITE;

  display.fillRoundRect(x, y, tileW, tileH, R, bg);
  display.setFont(LGFX_FONT);
  display.setTextColor(fg, bg);
  display.setTextWrap(false);

  String name = fitEllipsis(t.name, tileW - 12);
  display.setCursor(x+6, y+4); display.print(name);

  char buf[40];
  const char* sname = (t.st==UP) ? "UP" : (t.st==DOWN ? "DOWN" : "UNK");
  if (t.st==UP) snprintf(buf,sizeof(buf), "%s %ums", sname, (unsigned)t.lat_ms);
  else          snprintf(buf,sizeof(buf), "%s", sname);
  display.setCursor(x+6, y+4 + display.fontHeight()); display.print(buf);
}

void drawAllTiles(const TileInfo* arr, int n){
  for (int i=0;i<n;i++) drawTile(i, arr[i]);
}

bool insideTile(int idx, int sx, int sy){
  int x,y; tilePos(idx,x,y);
  return sx>=x && sx<=x+tileW && sy>=y && sy<=y+tileH;
}

void safeWipeAll(uint8_t finalRot){
  display.setClipRect(0, 0, display.width(), display.height());
  for (uint8_t r=0;r<4;r++){
    display.setRotation(r);
    display.fillScreen(TFT_BLACK);
    delay(2);
  }
  display.setRotation(finalRot);
  display.fillScreen(TFT_BLACK);
}

} // namespace UI
