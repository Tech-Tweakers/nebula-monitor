#include <Arduino.h>
#include "config.hpp"
#include "display.hpp"
#include "touch.hpp"
#include "net.hpp"
#include "scan.hpp"
#include <lvgl.h>

// Targets para network scanning
Target targets[] = {
  {"Google", "https://www.google.com", UNKNOWN, 0},
  {"Cloudflare", "https://1.1.1.1", UNKNOWN, 0},
  {"OpenDNS", "https://208.67.222.222", UNKNOWN, 0},
  {"Quad9", "https://9.9.9.9", UNKNOWN, 0},
  {"Nebula", "https://nebula.com", UNKNOWN, 0},
  {"GitHub", "https://github.com", UNKNOWN, 0}
};
static const int N_TARGETS = sizeof(targets) / sizeof(targets[0]);

// LVGL objects
static lv_obj_t* main_screen;
static lv_obj_t* title_label;
static lv_obj_t* tile_containers[N_TARGETS];

// Controle de refresh
static bool autoRefresh = true;
static uint32_t lastRefresh = 0;
static uint32_t nextRefresh = 0;
static const uint32_t AUTO_MS = 30000; // 30s

// Touch control
static uint32_t lastTapMs = 0;
static const uint32_t DEBOUNCE_MS = 200;

// ---------------- Helpers ----------------
static inline bool inTopBar(int sx, int sy){ return sy < 40; } // 40px para top bar

// ---------------- Declarações de UI ----------------
void drawTopBar(bool autoRefresh, uint32_t lastRefresh, int tileW, int tileH);
void drawTile(int idx, const Target& target, int cols, int tileW, int tileH);
void showFooter(const char* text, int tileW, int tileH);
bool insideTile(int idx, int sx, int sy, int cols, int tileW, int tileH);

// ---------------- Refresh All ----------------
void refreshAll(){
  lastRefresh = millis();
  int ok=0, fail=0;
  
  // Layout automático
  static const int COLS = 2;
  static const int ROWS = (N_TARGETS + COLS - 1) / COLS;  // 6 targets = 3 rows
  static const int TOP_BAR_HEIGHT = 50;
  static const int FOOTER_HEIGHT = 50;
  static const int AVAILABLE_HEIGHT = 320 - TOP_BAR_HEIGHT - FOOTER_HEIGHT;  // 220
  static const int TILE_W = 240 / COLS;  // 120
  static const int TILE_H = AVAILABLE_HEIGHT / ROWS; // ~73
  
  for (int i=0;i<N_TARGETS;i++){
    // Note: ScanManager now handles updating targets, this refreshAll is for UI update
    // For now, it still pings directly for simplicity in this function, but will be refactored.
    uint16_t ms = Net::httpPing(targets[i].url, 2500);
    if (ms>0){ targets[i].st=UP; targets[i].lat_ms=ms; ok++; }
    else     { targets[i].st=DOWN; targets[i].lat_ms=0;  fail++; }
    drawTile(i, targets[i], COLS, TILE_W, TILE_H);
  }
  char hud[64]; snprintf(hud,sizeof(hud), "OK:%d  FAIL:%d", ok, fail);
  drawTopBar(autoRefresh, lastRefresh, TILE_W, TILE_H);
  showFooter(hud, TILE_W, TILE_H);
  Serial.printf("[REFRESH] %s | interval=%lus\n", hud, AUTO_MS/1000);
}

void setup() {
  Serial.begin(115200); delay(60);
  Serial.println("[MAIN] Iniciando Nebula Monitor v3.0 (LVGL)");

  Serial.println("[MAIN] Inicializando display...");
  if (!initDisplay(ROT, BL_PIN)) {
    Serial.println("[MAIN] ERRO: Falha ao inicializar display!");
    return;
  }
  Serial.println("[MAIN] Display inicializado com sucesso!");

  // Create main screen
  main_screen = lv_obj_create(NULL);
  lv_scr_load(main_screen);
  
  // Set screen background
  lv_obj_set_style_bg_color(main_screen, lv_color_hex(0x000000), LV_PART_MAIN);
  
  // Create title bar with black background
  lv_obj_t* title_bar = lv_obj_create(main_screen);
  lv_obj_set_size(title_bar, 240, 30); // Full width, 30px height
  lv_obj_set_pos(title_bar, 0, 0);
  lv_obj_set_style_bg_color(title_bar, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_border_width(title_bar, 0, LV_PART_MAIN); // No border
  
  // Create title label in title bar
  title_label = lv_label_create(title_bar);
  lv_label_set_text(title_label, "Nebula Monitor v3.0");
  lv_obj_set_style_text_color(title_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_text_font(title_label, LV_FONT_DEFAULT, LV_PART_MAIN);
  lv_obj_align(title_label, LV_ALIGN_CENTER, 0, 0);
  
  // Create tile containers - LAYOUT AUTOMÁTICO
  int cols = 2;
  int rows = (N_TARGETS + cols - 1) / cols;
  
  // Calcular tamanho automático dos tiles
  int available_width = 240;  // Largura total da tela
  int available_height = 320 - 30;  // Altura total menos title bar
  
  int tile_w = (available_width - (cols + 1) * 5) / cols;  // Margem de 5px entre tiles
  int tile_h = (available_height - (rows + 1) * 5) / rows; // Margem de 5px entre tiles
  
  Serial.printf("[LAYOUT] Tela: %dx%d, Tiles: %dx%d, Grid: %dx%d\n", 
                available_width, available_height, tile_w, tile_h, cols, rows);
  
  for (int i = 0; i < N_TARGETS; i++) {
    int row = i / cols;
    int col = i % cols;
    int x = col * (tile_w + 5) + 5;  // Margem de 5px
    int y = row * (tile_h + 5) + 35; // 30px title bar + 5px margem
    
    tile_containers[i] = lv_obj_create(main_screen);
    lv_obj_set_size(tile_containers[i], tile_w, tile_h);
    lv_obj_set_pos(tile_containers[i], x, y);
    lv_obj_set_style_bg_color(tile_containers[i], lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_border_color(tile_containers[i], lv_color_hex(0x666666), LV_PART_MAIN);
    lv_obj_set_style_border_width(tile_containers[i], 2, LV_PART_MAIN);
    lv_obj_set_style_radius(tile_containers[i], 8, LV_PART_MAIN);
    
    // Create tile label
    lv_obj_t* tile_label = lv_label_create(tile_containers[i]);
    lv_label_set_text(tile_label, targets[i].name);
    lv_obj_set_style_text_color(tile_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(tile_label, LV_FONT_DEFAULT, LV_PART_MAIN);
    lv_obj_align(tile_label, LV_ALIGN_TOP_MID, 0, 5);
    
    // Create status label
    lv_obj_t* status_tile_label = lv_label_create(tile_containers[i]);
    lv_label_set_text(status_tile_label, "Unknown");
    lv_obj_set_style_text_color(status_tile_label, lv_color_hex(0xFFFF00), LV_PART_MAIN);
    lv_obj_set_style_text_font(status_tile_label, LV_FONT_DEFAULT, LV_PART_MAIN);
    lv_obj_align(status_tile_label, LV_ALIGN_BOTTOM_MID, 0, -5);
  }

  Touch::beginHSPI();
  if (Net::connectWiFi("Polaris","55548502")) {
    Net::printInfo(); Net::forceDNS(); Net::printInfo();
    bool ntp_ok = Net::ntpSync();
    Serial.printf("[WIFI] Status: %s\n", ntp_ok ? "Wi-Fi OK | NTP OK" : "Wi-Fi OK | NTP FAIL");
  } else {
    Serial.println("[WIFI] Status: Wi-Fi FAIL");
  }

  // Scanner desligado para teste da interface
  Serial.println("[MAIN] Scanner desligado - testando interface");

  Serial.printf("[BOOT] %dx%d rot=%u | Touch HSPI SCK=%d MOSI=%d MISO=%d CS=%d IRQ=%d\n",
                240, 320, ROT, T_SCK, T_MOSI, T_MISO, T_CS, T_IRQ);
}

void loop() {
  // Atualizar scanner (não-bloqueante) - DESLIGADO PARA TESTE
  // updateScanner();
  
  // Teste de touch nos tiles
  static bool touching = false;
  bool nowTouch = Touch::touched();
  
  if (nowTouch && !touching) {
    int16_t rx, ry, rz;
    Touch::readRaw(rx, ry, rz);
    int sx, sy;
    Touch::mapRawToScreen(rx, ry, sx, sy);
    
    // Verificar se tocou em algum tile
    for (int i = 0; i < N_TARGETS; i++) {
      lv_obj_t* tile = tile_containers[i];
      lv_area_t tile_area;
      lv_obj_get_coords(tile, &tile_area);
      
      if (sx >= tile_area.x1 && sx <= tile_area.x2 && 
          sy >= tile_area.y1 && sy <= tile_area.y2) {
        
        Serial.printf("[TOUCH] Tile %d (%s) tocado!\n", i, targets[i].name);
        
        // Gerar cor aleatória para o tile
        uint32_t randomColor = random(0x100000, 0xFFFFFF); // Cores RGB aleatórias
        lv_obj_set_style_bg_color(tile, lv_color_hex(randomColor), LV_PART_MAIN);
        
        // Atualizar texto de status com a cor em hex
        char statusText[16];
        snprintf(statusText, sizeof(statusText), "#%06X", randomColor);
        lv_obj_t* status_label = lv_obj_get_child(tile, 1);
        if (status_label) {
          lv_label_set_text(status_label, statusText);
        }
        
        // FORÇAR REFRESH DO TILE
        lv_obj_invalidate(tile);
        
        // TESTE RADICAL: Forçar refresh de toda a tela
        lv_obj_invalidate(main_screen);
        
        // TESTE: Mudar cor de fundo da tela também
        static uint32_t screenColor = 0x000000;
        screenColor = (screenColor == 0x000000) ? 0x111111 : 0x000000;
        lv_obj_set_style_bg_color(main_screen, lv_color_hex(screenColor), LV_PART_MAIN);
        
        Serial.printf("[TOUCH] Tile %d (%s) - Cor: #%06X - REFRESH FORÇADO + TELA\n", i, targets[i].name, randomColor);
        
        break;
      }
    }
  }
  touching = nowTouch;
  
  // Atualização automática da interface DESLIGADA para teste
  /*
  // Atualizar interface baseado nos resultados do scanner (simulado)
  static uint32_t lastUIUpdate = 0;
  if (millis() - lastUIUpdate > 3000) { // Atualizar UI a cada 3 segundos
    Serial.println("[UI] Atualizando interface...");
    
    for (int i = 0; i < N_TARGETS; i++) {
      // Simular resultado do scanner (teste)
      static int testCounter = 0;
      testCounter++;
      
      // Alternar entre UP e DOWN para teste
      bool isUp = (testCounter + i) % 2 == 0;
      
      uint32_t tileColor;
      const char* statusText;
      
      if (isUp) {
        tileColor = 0x00FF00; // Verde
        statusText = "UP";
        targets[i].st = UP;
        targets[i].lat_ms = 50 + (i * 20); // Latência simulada
      } else {
        tileColor = 0xFF0000; // Vermelho
        statusText = "DOWN";
        targets[i].st = DOWN;
        targets[i].lat_ms = 0;
      }
      
      // Atualizar cor do tile
      lv_obj_set_style_bg_color(tile_containers[i], lv_color_hex(tileColor), LV_PART_MAIN);
      
      // Atualizar texto de status
      lv_obj_t* status_label = lv_obj_get_child(tile_containers[i], 1);
      if (status_label) {
        lv_label_set_text(status_label, statusText);
      }
      
      Serial.printf("[UI] Tile %d: %s (%d ms)\n", i, statusText, targets[i].lat_ms);
    }
    
    lastUIUpdate = millis();
  }
  */
  
  // Handle LVGL tasks - FORÇAR RENDER
  renderFrame();
  
  // Debug: verificar se LVGL está funcionando
  static uint32_t lastDebug = 0;
  if (millis() - lastDebug > 5000) { // A cada 5 segundos
    Serial.println("[DEBUG] LVGL render funcionando...");
    
    // TESTE: Alternar cor do title bar para ver se algo muda
    static bool titleColor = false;
    titleColor = !titleColor;
    uint32_t newTitleColor = titleColor ? 0x333333 : 0x000000;
    lv_obj_set_style_bg_color(title_label, lv_color_hex(newTitleColor), LV_PART_MAIN);
    lv_obj_invalidate(title_label);
    
    Serial.printf("[DEBUG] Title color alternada: %s\n", titleColor ? "CINZA" : "PRETO");
    
    lastDebug = millis();
  }
  
  delay(10);
}

// ---------------- Funções de UI ----------------
void drawTopBar(bool autoRefresh, uint32_t lastRefresh, int tileW, int tileH) {
  DisplayManager::fillRect(0, 0, 240, 50, COL_BAR);
  char topText[64];
  snprintf(topText, sizeof(topText), "Nebula Monitor | Auto: %s", autoRefresh ? "ON" : "OFF");
  DisplayManager::drawText(10, 15, topText, COL_TXT, COL_BAR);
  DisplayManager::markDirty();
}

void drawTile(int idx, const Target& target, int cols, int tileW, int tileH) {
  int row = idx / cols;
  int col = idx % cols;
  int x = col * tileW;
  int y = 50 + row * tileH; // 50px para top bar
  
  uint16_t tileColor = (target.st == UP) ? COL_UP : (target.st == DOWN) ? COL_DOWN : COL_UNK;
  DisplayManager::fillRect(x, y, tileW, tileH, tileColor);
  DisplayManager::drawRect(x, y, tileW, tileH, COL_TXT);
  
  // Nome do target (centralizado horizontalmente)
  int textX = x + (tileW - strlen(target.name) * 6) / 2; // Aproximação da largura do texto
  DisplayManager::drawText(textX, y + 5, target.name, COL_TXT, tileColor);
  
  // Status e latência (centralizado horizontalmente)
  char statusText[32];
  if (target.st == UP) { 
    snprintf(statusText, sizeof(statusText), "UP %ums", target.lat_ms); 
  } else if (target.st == DOWN) { 
    snprintf(statusText, sizeof(statusText), "DOWN"); 
  } else { 
    snprintf(statusText, sizeof(statusText), "UNKNOWN"); 
  }
  int statusX = x + (tileW - strlen(statusText) * 6) / 2;
  DisplayManager::drawText(statusX, y + 25, statusText, COL_TXT, tileColor);
  
  DisplayManager::markDirty();
}

void showFooter(const char* text, int tileW, int tileH) {
  DisplayManager::fillRect(0, 270, 240, 50, COL_BAR); // 320-50 = 270
  DisplayManager::drawText(10, 285, text, COL_TXT, COL_BAR); // 270+15 = 285
  DisplayManager::markDirty();
}

bool insideTile(int idx, int sx, int sy, int cols, int tileW, int tileH) {
  int row = idx / cols;
  int col = idx % cols;
  int x = col * tileW;
  int y = 50 + row * tileH; // 50px para top bar
  return (sx >= x && sx < x + tileW && sy >= y && sy < y + tileH);
}
