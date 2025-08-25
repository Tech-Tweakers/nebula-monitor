#include <Arduino.h>
#include "config.hpp"
#include "ui.hpp"
#include "touch.hpp"
#include "net.hpp"

// Dados dos alvos
struct Target {
  const char* name;
  const char* url;
  Status st;
  uint16_t lat_ms;
};

Target targets[] = {
  {"Hyper-V/VMM", "https://192.168.1.128:8006/", UNKNOWN, 0},
  {"Grafana",       "http://192.168.1.128:3000/",  UNKNOWN, 0},
  {"Polaris API",   "https://endangered-musician-bolt-berlin.trycloudflare.com/inference/", UNKNOWN,0},
  {"Polaris INT",   "https://ebfc52323306.ngrok-free.app", UNKNOWN, 0},
  {"Polaris WEB",   "https://tech-tweakers.github.io/polaris-v2-web/", UNKNOWN, 0},
  {"Router #1",     "http://192.168.1.1/", UNKNOWN, 0},
  {"Router #2",     "http://192.168.1.172", UNKNOWN, 0}
};
const int N_TARGETS = sizeof(targets)/sizeof(targets[0]);

// topo / refresh
bool autoRefresh = true;
uint32_t nextRefresh=0, lastRefresh=0;
uint32_t AUTO_MS = 10000;
const uint32_t AUTO_CHOICES[] = {5000, 10000, 30000};
int AUTO_IDX = 1; // 10s

// toque na barra (long-press / short-press)
bool pressingBar = false;
uint32_t pressStart = 0;
bool longFired = false;

// helpers
inline bool inTopBar(int sy){ return sy < UI::barHeight(); }

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
  Net::connectWiFi(WIFI_SSID, WIFI_PASS);
  Net::printInfo();
  Net::forceDNS();
  Net::printInfo();

  bool ntp_ok = Net::ntpSync();
  Serial.println(ntp_ok ? "[NTP] OK" : "[NTP] FAIL");
  UI::showFooter(ntp_ok ? "Wi-Fi OK | NTP OK" : "Wi-Fi OK | NTP FAIL");

  // Primeira varredura
  refreshAll();
  nextRefresh = millis() + AUTO_MS;

  Serial.printf("[BOOT] %dx%d rot=%u | Touch HSPI SCK=%d MOSI=%d MISO=%d CS=%d IRQ=%d\n",
                UI::SW(), UI::SH(), ROT, T_SCK, T_MOSI, T_MISO, T_CS, T_IRQ);
}

void loop(){
  // Auto refresh
  if (autoRefresh && millis() >= nextRefresh){
    refreshAll();
    nextRefresh = millis() + AUTO_MS;
  }

  // Touch
  if (Touch::irqActive()){
    int16_t rx,ry,rz;
    if (Touch::readRaw(rx,ry,rz)){
      int sx,sy; Touch::mapRawToScreen(rx,ry,sx,sy, UI::SW(), UI::SH());

      if (inTopBar(sy)){
        // início do toque na barra
        if (!pressingBar){
          pressingBar = true;
          pressStart = millis();
          longFired  = false;
        }

        // long-press: troca intervalo
        if (!longFired && millis() - pressStart > 1000){
          AUTO_IDX = (AUTO_IDX + 1) % (int)(sizeof(AUTO_CHOICES)/sizeof(AUTO_CHOICES[0]));
          AUTO_MS  = AUTO_CHOICES[AUTO_IDX];
          char buf[32];
          snprintf(buf, sizeof(buf), "Auto interval: %lus", (unsigned long)(AUTO_MS/1000));
          UI::showFooter(buf);
          longFired = true;
          Serial.printf("[AUTO] interval -> %lus\n", AUTO_MS/1000);
        }

      } else {
        // clique em tile (debounce simples)
        static uint32_t lastTap=0; const uint16_t DEBOUNCE_MS=120;
        uint32_t now = millis();
        if (now - lastTap > DEBOUNCE_MS){
          for (int i=0;i<N_TARGETS;i++){
            if (UI::insideTile(i, sx, sy)){
              uint16_t ms = Net::httpPing(targets[i].url);
              targets[i].st = (ms>0)?UP:DOWN;
              targets[i].lat_ms = (ms>0)?ms:0;
              TileInfo ti{targets[i].name, targets[i].st, targets[i].lat_ms};
              UI::drawTile(i, ti);
              UI::drawTopBar(autoRefresh, lastRefresh);
              char hud[64];
              snprintf(hud,sizeof(hud), "%s: %s %ums",
                       targets[i].name, targets[i].st==UP?"UP":"DOWN",
                       (unsigned)targets[i].lat_ms);
              UI::showFooter(hud);
              Serial.printf("[ONE] %s -> %s (%ums)\n",
                            targets[i].name, targets[i].st==UP?"UP":"DOWN",
                            (unsigned)targets[i].lat_ms);
              break;
            }
          }
          lastTap = now;
        }
      }
    }
  } else {
    // soltou toque — se estava na barra e NÃO foi long-press, é toggle ON/OFF
    if (pressingBar){
      if (!longFired){
        autoRefresh = !autoRefresh;
        UI::drawTopBar(autoRefresh, lastRefresh);
        refreshAll();
        nextRefresh = millis() + AUTO_MS;
        Serial.printf("[AUTO] %s | %lus\n", autoRefresh?"ON":"OFF", AUTO_MS/1000);
      }
      pressingBar = false;
      longFired = false;
    }
  }
}
