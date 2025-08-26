#include <Arduino.h>
#include "config.hpp"
#include "ui.hpp"
#include "touch.hpp"
#include "net.hpp"

// ---------------- Watch targets ----------------
struct Target {
  const char* name;
  const char* url;
  Status  st;
  uint16_t lat_ms;
};

Target targets[] = {
  {"Hyper-V/VMM", "https://192.168.1.128:8006/", UNKNOWN, 0},
  {"Grafana",       "http://192.168.1.128:3000/",  UNKNOWN, 0},
  {"Polaris API",   "https://endangered-musician-bolt-berlin.trycloudflare.com/inference/", UNKNOWN,0},
  {"Polaris INT",   "https://ebfc52323306.ngrok-free.app", UNKNOWN, 0},
  {"Polaris WEB",   "https://tech-tweakers.github.io/polaris-v2-web/", UNKNOWN, 0},
  {"DNS Server",    "https://tech-tweakers.github.io/polaris-v2-web/", UNKNOWN, 0},
  {"Router #1",     "http://192.168.1.1/", UNKNOWN, 0},
  {"Router #2",     "http://192.168.1.172", UNKNOWN, 0}
};
static const int N_TARGETS = sizeof(targets)/sizeof(targets[0]);

// ---------------- Layout/control ----------------
static bool     autoRefresh = true;
static uint32_t nextRefresh = 0, lastRefresh = 0;
static const uint32_t AUTO_MS = 10000;
static const uint16_t DEBOUNCE_MS = 120;
static uint32_t lastTapMs = 0;

// ---------------- Helpers ----------------
static inline bool inTopBar(int sx, int sy){ return sy < UI::barHeight(); }

// ---------------- Refresh All ----------------
void refreshAll(){
  lastRefresh = millis();
  int ok=0, fail=0;
  for (int i=0;i<N_TARGETS;i++){
    uint16_t ms = Net::httpPing(targets[i].url, 2500);
    if (ms>0){ targets[i].st=UP; targets[i].lat_ms=ms; ok++; }
    else     { targets[i].st=DOWN; targets[i].lat_ms=0;  fail++; }
    TileInfo ti{targets[i].name, targets[i].st, targets[i].lat_ms};
    UI::drawTile(i, ti);
  }
  char hud[64]; snprintf(hud,sizeof(hud), "OK:%d  FAIL:%d", ok, fail);
  UI::drawTopBar(autoRefresh, lastRefresh);
  UI::showFooter(hud);
  Serial.printf("[REFRESH] %s | interval=%lus\n", hud, AUTO_MS/1000);
}

// ---------------- Setup ----------------
void setup(){
  Serial.begin(115200);
  delay(60);

  // UI
  UI::begin(ROT, BL_PIN);
  UI::safeWipeAll(ROT);
  UI::setCols(2);
  UI::computeGrid(N_TARGETS);
  UI::drawTopBar(autoRefresh, lastRefresh);
  for (int i=0;i<N_TARGETS;i++){
    TileInfo ti{targets[i].name, targets[i].st, targets[i].lat_ms};
    UI::drawTile(i, ti);
  }
  UI::showFooter("Inicializando...");

  // Touch
  Touch::beginHSPI();

  // Net
  if (Net::connectWiFi(WIFI_SSID, WIFI_PASS)) {
    Net::printInfo();
    Net::forceDNS();       // 8.8.8.8 + 1.1.1.1
    Net::printInfo();
    bool ntp_ok = Net::ntpSync();
    UI::showFooter(ntp_ok ? "Wi-Fi OK | NTP OK" : "Wi-Fi OK | NTP FAIL");
  } else {
    UI::showFooter("Wi-Fi FAIL");
  }

  // Primeira varredura
  refreshAll();
  nextRefresh = millis() + AUTO_MS;

  Serial.printf("[BOOT] %dx%d rot=%u | Touch HSPI SCK=%d MOSI=%d MISO=%d CS=%d IRQ=%d\n",
                UI::SW(), UI::SH(), ROT, T_SCK, T_MOSI, T_MISO, T_CS, T_IRQ);
}

// ---------------- Loop ----------------
void loop(){
  // Auto-refresh
  if (autoRefresh && millis() >= nextRefresh){
    refreshAll();
    nextRefresh = millis() + AUTO_MS;
  }

  // Touch
  static bool touching=false;
  bool nowTouch = Touch::touched();
  if (nowTouch && !touching){
    int16_t rx,ry,rz; Touch::readRaw(rx,ry,rz);
    int sx, sy; Touch::mapRawToScreen(rx,ry,sx,sy);

    if (inTopBar(sx, sy)){
      autoRefresh = !autoRefresh;
      UI::drawTopBar(autoRefresh, lastRefresh);
      refreshAll();
      nextRefresh = millis() + AUTO_MS;
      Serial.printf("[AUTO] %s\n", autoRefresh ? "ON" : "OFF");
    } else {
      uint32_t nowMs = millis();
      if (nowMs - lastTapMs > DEBOUNCE_MS){
        for (int i=0;i<N_TARGETS;i++){
          if (UI::insideTile(i, sx, sy)){
            uint16_t ms = Net::httpPing(targets[i].url, 2500);
            if (ms>0){ targets[i].st=UP; targets[i].lat_ms=ms; }
            else     { targets[i].st=DOWN; targets[i].lat_ms=0; }
            TileInfo ti{targets[i].name, targets[i].st, targets[i].lat_ms};
            UI::drawTile(i, ti);
            UI::drawTopBar(autoRefresh, lastRefresh);
            char hud[64];
            snprintf(hud,sizeof(hud), "%s: %s %ums", targets[i].name,
                     targets[i].st==UP?"UP":"DOWN", (unsigned)targets[i].lat_ms);
            UI::showFooter(hud);
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
