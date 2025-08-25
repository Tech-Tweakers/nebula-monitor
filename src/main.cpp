#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

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

// ==================== LCD (Lovyan autodetect) ====================
static LGFX display;
static constexpr auto LGFX_FONT = &fonts::Font0;

static const int BL_PIN = 27;
static const uint8_t ROT = 2;      // validado
int16_t SW = 240, SH = 320;        // será lido do display

// ==================== Touch XPT2046 (HSPI compartilhado) ====================
static const int T_SCK  = 14;      // HSPI SCK
static const int T_MOSI = 13;      // HSPI MOSI
static const int T_MISO = 12;      // HSPI MISO
static const int T_CS   = 33;      // CS do touch
static const int T_IRQ  = 36;      // IRQ ativo LOW

SPIClass hspi(HSPI);
XPT2046_Touchscreen tp(T_CS, T_IRQ);

// ---- Calibração bruta (mantemos tua) ----
static int RAW_X_MIN = 200,  RAW_X_MAX = 3700;
static int RAW_Y_MIN = 240,  RAW_Y_MAX = 3800;

// ==================== Wi-Fi ====================
const char* WIFI_SSID = "Polaris";
const char* WIFI_PASS = "55548502";

// ==================== Palette / cores ====================
static inline uint16_t RGB(uint8_t r,uint8_t g,uint8_t b){
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
#define COL_BG    TFT_BLACK
#define COL_BAR   TFT_DARKGREY
#define COL_TXT   TFT_WHITE
#define COL_UP    0x07E0     // verde
#define COL_DOWN  0xF800     // vermelho
#define COL_UNK   0x8410     // cinza

// ==================== Watchdog ====================
enum Status : uint8_t { UNKNOWN=0, UP=1, DOWN=2 };

struct Target {
  const char* name;
  const char* url;
  Status  st;
  uint16_t lat_ms;
};

Target targets[] = {
  {"Constellation",    "https://192.168.1.128:8006/", UNKNOWN, 0},
  {"Grafana",          "http://192.168.1.128:3000/", UNKNOWN, 0},
  {"Polaris API",      "https://endangered-musician-bolt-berlin.trycloudflare.com", UNKNOWN, 0},
  {"Polaris Integ",    "https://ebfc52323306.ngrok-free.app", UNKNOWN, 0}
};
const int N_TARGETS = sizeof(targets)/sizeof(targets[0]);

// ==================== Layout ====================
static const int BAR_H = 30;     // barra superior (auto ON/OFF + info)
static const int PAD   = 8;      // padding geral
static const int R     = 6;      // raio dos cantos
static const int COLS  = 2;      // grid 2 colunas

int rows=0, tileW=0, tileH=0, gridTop=0;

void computeGrid(){
  rows   = (N_TARGETS + COLS - 1) / COLS;
  gridTop= BAR_H + PAD;                          // abaixo da barra
  tileW  = (SW - (COLS + 1)*PAD) / COLS;
  int availH = SH - gridTop - (rows + 1)*PAD - 16; // 16px pro rodapé
  tileH  = max(34, availH / rows);
}

// ==================== Topo / rodapé ====================
bool autoRefresh = true;
uint32_t nextRefresh = 0, lastRefresh = 0;
const uint32_t AUTO_MS = 10000;      // 10s
const uint16_t HTTP_TIMEOUT_MS = 2500;

void drawTopBar(){
  display.fillRect(0,0,SW,BAR_H, COL_BAR);
  display.setFont(LGFX_FONT);
  display.setTextColor(COL_TXT, COL_BAR);
  char buf[64];
  snprintf(buf,sizeof(buf), "Auto: %s  (toque)", autoRefresh ? "ON" : "OFF");
  int16_t tw = display.textWidth(buf);
  display.setCursor(PAD,  (BAR_H - display.fontHeight())/2); display.print(buf);

  // carimbo de última atualização
  snprintf(buf,sizeof(buf), "t=%lus", (unsigned long)(lastRefresh/1000));
  int16_t tw2 = display.textWidth(buf);
  display.setCursor(SW - PAD - tw2, (BAR_H - display.fontHeight())/2); display.print(buf);
}

void showFooter(const char* msg){
  int footerH = 16;
  display.fillRect(0, SH - footerH, SW, footerH, COL_BG);
  display.setFont(LGFX_FONT);
  display.setTextColor(COL_TXT, COL_BG);
  int16_t tw = display.textWidth(msg);
  display.setCursor(SW/2 - tw/2, SH - footerH + (footerH - display.fontHeight())/2);
  display.print(msg);
}

// ==================== Tiles ====================
void tilePos(int idx, int& x, int& y){
  int r = idx / COLS;
  int c = idx % COLS;
  x = PAD + c*(tileW + PAD);
  y = gridTop + PAD + r*(tileH + PAD);
}

void drawTile(int idx){
  int x,y; tilePos(idx,x,y);
  Status st = targets[idx].st;
  uint16_t bg = (st==UP) ? COL_UP : (st==DOWN ? COL_DOWN : COL_UNK);

  display.fillRoundRect(x, y, tileW, tileH, R, bg);
  display.setFont(LGFX_FONT);
  display.setTextColor(TFT_WHITE, bg);

  // nome
  display.setCursor(x+6, y+4);
  display.print(targets[idx].name);

  // status / latência
  char buf[32];
  const char* sname = (st==UP) ? "UP" : (st==DOWN ? "DOWN" : "UNK");
  if (st==UP) snprintf(buf,sizeof(buf), "%s %ums", sname, (unsigned)targets[idx].lat_ms);
  else        snprintf(buf,sizeof(buf), "%s", sname);
  display.setCursor(x+6, y+4 + display.fontHeight());
  display.print(buf);
}

void drawAllTiles(){ for (int i=0;i<N_TARGETS;i++) drawTile(i); }

bool insideTile(int idx, int sx, int sy){
  int x,y; tilePos(idx,x,y);
  return sx>=x && sx<=x+tileW && sy>=y && sy<=y+tileH;
}

// ==================== Rede ====================
uint16_t httpCheck(const char* url){
  if (WiFi.status() != WL_CONNECTED) return 0;
  HTTPClient http; http.setTimeout(HTTP_TIMEOUT_MS);
  uint32_t t0 = millis();
  if (!http.begin(url)) return 0;
  int code = http.GET(); uint32_t dt = millis() - t0;
  http.end();
  if (code == HTTP_CODE_OK) {
    if (dt > 65535) dt = 65535;
    return (uint16_t)dt;
  }
  return 0;
}

void refreshAll(){
  lastRefresh = millis();
  int ok=0, fail=0;
  for (int i=0;i<N_TARGETS;i++){
    uint16_t ms = httpCheck(targets[i].url);
    if (ms > 0){ targets[i].st=UP;   targets[i].lat_ms=ms; ok++; }
    else       { targets[i].st=DOWN; targets[i].lat_ms=0;  fail++; }
    drawTile(i);
  }
  char hud[64];
  snprintf(hud,sizeof(hud), "OK:%d  FAIL:%d", ok, fail);
  drawTopBar();
  showFooter(hud);
  Serial.printf("[REFRESH] %s\n", hud);
}

// ==================== Touch (MAP 4: SWAP_XY fixo) ====================
bool touching=false;
uint32_t lastTapMs=0;
const uint16_t DEBOUNCE_MS=120;

void mapRawToScreen(int16_t rx,int16_t ry,int& sx,int& sy){
  // swap cru (MAP 4)
  int16_t xraw = ry;
  int16_t yraw = rx;
  // ranges trocados
  long nx = map(xraw, RAW_Y_MIN, RAW_Y_MAX, 0, SW-1);
  long ny = map(yraw, RAW_X_MIN, RAW_X_MAX, 0, SH-1);
  sx = constrain((int)nx, 0, SW-1);
  sy = constrain((int)ny, 0, SH-1);
}

bool inTopBar(int sx, int sy){ return sy < BAR_H; }

// ==================== Setup ====================
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

void connectWiFi(){
  display.fillScreen(COL_BG);
  display.setFont(LGFX_FONT);
  display.setTextColor(COL_TXT, COL_BG);
  const char* msg = "Conectando Wi-Fi...";
  int16_t tw = display.textWidth(msg);
  display.setCursor(SW/2 - tw/2, SH/2 - display.fontHeight()/2);
  display.print(msg);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000){
    delay(250);
    Serial.print(".");
  }
  Serial.println(WiFi.status()==WL_CONNECTED ? "\nWiFi OK" : "\nWiFi falhou");
}

void setup(){
  Serial.begin(115200);
  delay(60);

  // LCD
  display.begin();
  pinMode(BL_PIN, OUTPUT); digitalWrite(BL_PIN, HIGH);
  display.setColorDepth(16);
  display.setRotation(ROT);
  SW = display.width(); SH = display.height();
  safeWipeAll(ROT);

  computeGrid();
  drawTopBar();
  drawAllTiles();
  showFooter("Inicializando...");

  // Touch
  pinMode(T_CS, OUTPUT);  digitalWrite(T_CS, HIGH);
  pinMode(T_IRQ, INPUT);
  hspi.end();
  hspi.begin(T_SCK, T_MISO, T_MOSI, T_CS);
  tp.begin(hspi);
  tp.setRotation(0);   // orientação fica com nosso map fixo

  // Wi-Fi
  connectWiFi();

  // Primeira varredura
  refreshAll();
  nextRefresh = millis() + AUTO_MS;

  Serial.printf("[BOOT] %dx%d rot=%u | Touch HSPI SCK=%d MOSI=%d MISO=%d CS=%d IRQ=%d\n",
                SW, SH, ROT, T_SCK, T_MOSI, T_MISO, T_CS, T_IRQ);
}

// ==================== Loop ====================
void loop(){
  // Auto refresh
  if (autoRefresh && millis() >= nextRefresh){
    refreshAll();
    nextRefresh = millis() + AUTO_MS;
  }

  // Touch gate por IRQ
  bool nowTouch = (digitalRead(T_IRQ) == LOW);
  if (nowTouch && !touching){
    TS_Point p = tp.getPoint();
    int sx, sy; mapRawToScreen(p.x, p.y, sx, sy);

    // Clique na barra -> toggle auto + refresh
    if (inTopBar(sx, sy)){
      autoRefresh = !autoRefresh;
      drawTopBar();
      refreshAll();                 // feedback imediato
      nextRefresh = millis() + AUTO_MS;
      Serial.printf("[AUTO] %s\n", autoRefresh ? "ON" : "OFF");
    } else {
      // Clique em tile -> refresh individual
      uint32_t nowMs = millis();
      if (nowMs - lastTapMs > DEBOUNCE_MS){
        for (int i=0;i<N_TARGETS;i++){
          if (insideTile(i, sx, sy)){
            uint16_t ms = httpCheck(targets[i].url);
            if (ms > 0){ targets[i].st=UP; targets[i].lat_ms=ms; }
            else       { targets[i].st=DOWN; targets[i].lat_ms=0; }
            drawTile(i);
            drawTopBar();
            char hud[64];
            snprintf(hud,sizeof(hud), "%s: %s %ums",
                     targets[i].name, targets[i].st==UP?"UP":"DOWN", (unsigned)targets[i].lat_ms);
            showFooter(hud);
            Serial.printf("[ONE] %s -> %s (%ums)\n", targets[i].name,
                          targets[i].st==UP?"UP":"DOWN", (unsigned)targets[i].lat_ms);
            break;
          }
        }
        lastTapMs = nowMs;
      }
    }
  }
  touching = nowTouch;
}
