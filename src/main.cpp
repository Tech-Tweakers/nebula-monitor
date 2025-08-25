#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

TFT_eSPI tft = TFT_eSPI();

// --- Touch XPT2046 (SPI dedicado)
#define XPT2046_IRQ  36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK  25
#define XPT2046_CS   33
SPIClass touchscreenSPI(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

// --- WiFi (preencher)
const char* WIFI_SSID = "Polaris";
const char* WIFI_PASS = "55548502";

// --- Dimensões dinâmicas
int16_t SW = 320, SH = 240;

// --- Botão topo
#define BTN_WIDTH   150
#define BTN_HEIGHT   32
#define BTN_Y         6
int BTN_X = 0;

// --- Backlight
static const int BL_PIN = 27;
static inline void setBL(bool on){ pinMode(BL_PIN, OUTPUT); digitalWrite(BL_PIN, on ? HIGH : LOW); }

// --- Paleta
#define COL_BG   TFT_BLACK
#define COL_BTN  0x528A
#define COL_UP   0x07E0
#define COL_DOWN 0xF800
#define COL_UNK  0x8410

// --- Fonte garantida
#define FONT_ID 1  // GLCD

// --- Alvos
struct Target { const char* name; const char* url; };
Target targets[] = {
  {"Polaris API",      "http://10.10.10.60:8000/health"},
  {"Integrations",     "http://10.10.10.61:8001/health"},
  {"Prometheus",       "http://10.10.10.50:9090/-/ready"},
  {"Pushgateway",      "http://10.10.10.50:9091/-/ready"},
  {"Grafana",          "http://10.10.10.50:3000/api/health"},
  {"ChromaDB",         "http://10.10.10.62:8002/health"},
  {"Mongo Exporter",   "http://10.10.10.62:9216/metrics"},
  {"Telegram Bot",     "http://10.10.10.64:8080/health"}
};
const int N_TARGETS = sizeof(targets)/sizeof(targets[0]);

// --- Estado
enum Status : uint8_t { UNKNOWN=0, UP=1, DOWN=2 };
Status   statArr[16];
uint16_t latMs[16];
bool     autoRefresh = true;

// --- Grid (fixo 2 colunas)
const int PAD = 8;
const int FOOTER_H = 18;
const int MIN_TILE_H = 30;
int gridTop = 0, cols = 2, rows = 0, tileW = 0, tileH = 0;

// --- Touch
bool touching = false;

// --- Refresh
uint32_t nextRefresh = 0, lastRefresh = 0;
const uint32_t AUTO_MS = 10000;
const uint16_t HTTP_TIMEOUT_MS = 2000;

// Limpa fantasmas SEM resetar o painel: varre todas as rotações
void safeWipeAll(uint8_t finalRot = 2){
  tft.resetViewport();          // remove qualquer clipping pendente
  for (uint8_t r = 0; r < 4; r++){
    tft.setRotation(r);
    tft.fillScreen(TFT_BLACK);
    delay(2);
  }
  tft.setRotation(finalRot);
  tft.fillScreen(TFT_BLACK);
}


// ---------------- UI helpers ----------------
void computeGrid(){
  rows  = (N_TARGETS + cols - 1) / cols;  // 2 colunas
  gridTop = BTN_Y + BTN_HEIGHT + 8;
  tileW = (SW - (cols + 1) * PAD) / cols;
  int availH = SH - gridTop - (rows + 1) * PAD - FOOTER_H;
  tileH = availH / rows;
  if (tileH < MIN_TILE_H) tileH = MIN_TILE_H;
}

void drawFrameDebug(){ tft.drawRect(0, 0, SW, SH, TFT_WHITE); }

void drawTopButton(){
  uint16_t c = autoRefresh ? 0x1A9F : COL_BTN;
  tft.fillRoundRect(BTN_X, BTN_Y, BTN_WIDTH, BTN_HEIGHT, 8, c);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(FONT_ID);
  tft.setTextColor(TFT_BLACK, c);
  tft.drawCentreString(autoRefresh ? "Auto: ON" : "Auto: OFF",
                       BTN_X + BTN_WIDTH/2, BTN_Y + BTN_HEIGHT/2 - 5, FONT_ID);
}

void drawTile(int idx){
  int r = idx / cols, c = idx % cols;
  int x = PAD + c*(tileW + PAD);
  int y = gridTop + PAD + r*(tileH + PAD);

  Status st = statArr[idx];
  uint16_t bg = (st==UP) ? COL_UP : (st==DOWN ? COL_DOWN : COL_UNK);
  tft.fillRoundRect(x, y, tileW, tileH, 6, bg);

  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(FONT_ID);
  tft.setTextColor(TFT_WHITE, bg);
  tft.drawString(targets[idx].name, x+6, y+4, FONT_ID);

  char buf[48];
  const char* sname = (st==UP) ? "UP" : (st==DOWN ? "DOWN" : "UNK");
  if (st==UP) snprintf(buf, sizeof(buf), "%s  %u ms", sname, (unsigned)latMs[idx]);
  else        snprintf(buf, sizeof(buf), "%s", sname);
  tft.drawString(buf, x+6, y+4+12, FONT_ID);
}

void drawAllTiles(){ for (int i=0;i<N_TARGETS;i++) drawTile(i); }

void showFooter(const char* msg){
  tft.fillRect(0, SH - FOOTER_H, SW, FOOTER_H, COL_BG);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(FONT_ID);
  tft.setTextColor(TFT_WHITE, COL_BG);
  tft.drawCentreString(msg, SW/2, SH - FOOTER_H/2, FONT_ID);
}

// ---------------- NET ----------------
uint16_t httpCheck(const char* url){
  if (WiFi.status() != WL_CONNECTED) return 0;
  HTTPClient http; http.setTimeout(HTTP_TIMEOUT_MS);
  uint32_t t0 = millis();
  if (!http.begin(url)) return 0;
  int code = http.GET(); uint32_t dt = millis() - t0;
  http.end();
  if (code == HTTP_CODE_OK) { if (dt > 65535) dt = 65535; return (uint16_t)dt; }
  return 0;
}

void refreshAll(){
  lastRefresh = millis();
  int ok=0, fail=0;
  for (int i=0;i<N_TARGETS;i++){
    uint16_t ms = httpCheck(targets[i].url);
    if (ms > 0){ statArr[i]=UP; latMs[i]=ms; ok++; }
    else       { statArr[i]=DOWN; latMs[i]=0;   fail++; }
    drawTile(i);
  }
  char hud[64];
  snprintf(hud, sizeof(hud), "Atualizado: %lu  |  OK:%d  FAIL:%d",
           (unsigned long)(lastRefresh/1000), ok, fail);
  showFooter(hud);
  Serial.println(hud);
}

// ---------------- Setup / Loop ----------------
void applyRotation(uint8_t r){
  tft.setRotation(r);
  touchscreen.setRotation(r);
  SW = tft.width(); SH = tft.height();
  BTN_X = (SW - BTN_WIDTH) / 2;
  computeGrid();
}

bool isTouchInsideButton(int x, int y){
  return x >= BTN_X && x <= (BTN_X + BTN_WIDTH) &&
         y >= BTN_Y && y <= (BTN_Y + BTN_HEIGHT);
}

void connectWiFi(){
  tft.fillScreen(COL_BG);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(FONT_ID);
  tft.setTextColor(TFT_WHITE, COL_BG);
  tft.drawCentreString("Conectando Wi-Fi...", SW/2, SH/2, FONT_ID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000){
    delay(250); Serial.print(".");
  }
  Serial.println(WiFi.status()==WL_CONNECTED ? "\nWiFi OK" : "\nWiFi falhou");
}

void setup() {
  Serial.begin(115200);
  delay(100);

  setBL(false); // sem flash durante init

  // Touch
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);

  // TFT
  tft.init();
  applyRotation(0);      // já define SW/SH e rot do touch
  safeWipeAll(0);        // limpa SEM resetar a controladora
  tft.fillScreen(COL_BG);


  // Info de debug na tela
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(FONT_ID);
  tft.setTextColor(TFT_WHITE, COL_BG);
  char wh[24]; snprintf(wh, sizeof(wh), "%dx%d", SW, SH);
  tft.drawString(wh, 2, 2, FONT_ID);
  drawFrameDebug();

  // WiFi + UI
  connectWiFi();
  //drawTopButton();
  for (int i=0;i<N_TARGETS;i++){ statArr[i]=UNKNOWN; latMs[i]=0; }
  drawAllTiles();
  showFooter("Aguardando primeira varredura...");

  setBL(true);

  refreshAll();
  nextRefresh = millis() + AUTO_MS;

  Serial.printf("Display: %dx%d (rot=2)\n", SW, SH);
}

void loop() {
  if (autoRefresh && millis() >= nextRefresh){
    refreshAll();
    nextRefresh = millis() + AUTO_MS;
  }

  bool nowTouch = touchscreen.touched();
  if (nowTouch && !touching){
    TS_Point p = touchscreen.getPoint();
    int x = map(p.x, 200, 3700, 1, SW);
    int y = map(p.y, 240, 3800, 1, SH);
    x = SW - x; y = SH - y; // rot=2

    if (isTouchInsideButton(x,y)){
      autoRefresh = !autoRefresh;
      //drawTopButton();
      refreshAll();
      nextRefresh = millis() + AUTO_MS;
    }
  }
  touching = nowTouch;
}
