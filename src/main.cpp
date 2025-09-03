#include <Arduino.h>
#include <WiFi.h>
#include "config.hpp"
#include "display.hpp"
#include "touch.hpp"
#include "net.hpp"
#include "scan.hpp"
#include "telegram.hpp"
#include <lvgl.h>

// Network targets - serão carregados dinamicamente do config.env
static Target targets[10]; // Array dinâmico para targets
static int N_TARGETS = 0;  // Número de targets carregados

// RGB System Status LED config (runtime from ConfigManager)
static int LED_PIN_R;
static int LED_PIN_G;
static int LED_PIN_B;
static bool LED_ACTIVE_HIGH;

// PWM (LEDC) configuration for dimming
static const int LEDC_CHANNEL_R = 0;
static const int LEDC_CHANNEL_G = 1;
static const int LEDC_CHANNEL_B = 2;
static int LEDC_FREQ;       // Hz
static int LEDC_RES_BITS;   // bits

// Per-channel base brightness (0-255). Medium-to-weak overall
static int LED_BRIGHT_R;
static int LED_BRIGHT_G;
static int LED_BRIGHT_B;

// Track last LED state to avoid redundant writes
static int last_led_r = -1;
static int last_led_g = -1;
static int last_led_b = -1;

inline void setStatusLed(bool r_on, bool g_on, bool b_on) {
  // Compute target brightness per channel
  const int r_brightness = r_on ? LED_BRIGHT_R : 0;
  const int g_brightness = g_on ? LED_BRIGHT_G : 0;
  const int b_brightness = b_on ? LED_BRIGHT_B : 0;

  if (last_led_r == r_brightness && last_led_g == g_brightness && last_led_b == b_brightness) return;
  last_led_r = r_brightness; last_led_g = g_brightness; last_led_b = b_brightness;

  // Invert for common anode if needed
  const int r_duty = LED_ACTIVE_HIGH ? r_brightness : (255 - r_brightness);
  const int g_duty = LED_ACTIVE_HIGH ? g_brightness : (255 - g_brightness);
  const int b_duty = LED_ACTIVE_HIGH ? b_brightness : (255 - b_brightness);

  ledcWrite(LEDC_CHANNEL_R, r_duty);
  ledcWrite(LEDC_CHANNEL_G, g_duty);
  ledcWrite(LEDC_CHANNEL_B, b_duty);
}

inline void updateStatusLed() {
  bool isScanning = ScanManager::isActive();
  bool isTelegramSending = TelegramAlerts::isSendingMessage();
  bool hasActiveAlerts = TelegramAlerts::hasActiveAlerts();

  if (isTelegramSending) {
    // Blue
    setStatusLed(false, false, true);
  } else if (isScanning) {
    // Red
    setStatusLed(true, false, false);
  } else if (hasActiveAlerts) {
    // Yellow (Red + Green)
    setStatusLed(true, true, false);
  } else {
    // Green (idle/ok)
    setStatusLed(false, true, false);
  }
}

// Função para carregar targets do ConfigManager
void loadTargetsFromConfig() {
  N_TARGETS = ConfigManager::getTargetCount();
  Serial.printf("[MAIN] Carregando %d targets do config.env\n", N_TARGETS);
  
  // Verificação de segurança - se não há targets, usar valores padrão
  if (N_TARGETS == 0) {
    Serial.println("[MAIN] AVISO: Nenhum target encontrado no config.env, usando valores padrão!");
    N_TARGETS = 6; // Usar 6 targets padrão
    
    // Targets padrão hardcoded como fallback
    const char* defaultTargets[][4] = {
      {"Proxmox HV", "http://192.168.1.128:8006/", "", "PING"},
      {"Router #1", "http://192.168.1.1", "", "PING"},
      {"Router #2", "https://192.168.1.172", "", "PING"},
      {"Polaris API", "https://pet-chem-independence-australia.trycloudflare.com", "/health", "HEALTH_CHECK"},
      {"Polaris INT", "http://ebfc52323306.ngrok-free.app", "/health", "PING"},
      {"Polaris WEB", "https://tech-tweakers.github.io/polaris-v2-web", "", "PING"}
    };
    
    for (int i = 0; i < N_TARGETS; i++) {
      targets[i].name = strdup(defaultTargets[i][0]);
      targets[i].url = strdup(defaultTargets[i][1]);
      targets[i].health_endpoint = strlen(defaultTargets[i][2]) > 0 ? strdup(defaultTargets[i][2]) : nullptr;
      targets[i].monitor_type = strcmp(defaultTargets[i][3], "HEALTH_CHECK") == 0 ? HEALTH_CHECK : PING;
      targets[i].st = UNKNOWN;
      targets[i].lat_ms = 0;
      
      Serial.printf("[MAIN] Target padrão %d: %s | %s | %s | %s\n", 
                   i, defaultTargets[i][0], defaultTargets[i][1], 
                   strlen(defaultTargets[i][2]) > 0 ? defaultTargets[i][2] : "null",
                   defaultTargets[i][3]);
    }
    return;
  }
  
  for (int i = 0; i < N_TARGETS; i++) {
    String name = ConfigManager::getTargetName(i);
    String url = ConfigManager::getTargetUrl(i);
    String healthEndpoint = ConfigManager::getTargetHealthEndpoint(i);
    String monitorType = ConfigManager::getTargetMonitorType(i);
    
    // Converter string para MonitorType
    MonitorType type = PING;
    if (monitorType.equalsIgnoreCase("HEALTH_CHECK")) {
      type = HEALTH_CHECK;
    }
    
    // Alocar strings dinamicamente
    targets[i].name = strdup(name.c_str());
    targets[i].url = strdup(url.c_str());
    targets[i].health_endpoint = healthEndpoint.length() > 0 ? strdup(healthEndpoint.c_str()) : nullptr;
    targets[i].monitor_type = type;
    targets[i].st = UNKNOWN;
    targets[i].lat_ms = 0;
    
    Serial.printf("[MAIN] Target %d: %s | %s | %s | %s\n", 
         i, name.c_str(), url.c_str(), 
         healthEndpoint.length() > 0 ? healthEndpoint.c_str() : "null",
         monitorType.c_str());
  }
}

// LVGL objects
static lv_obj_t* main_screen;
static lv_obj_t* title_label;
static lv_obj_t* tile_containers[6]; // Fixed size for now
static lv_obj_t* status_labels[6]; // Status items for scanner updates
static lv_obj_t* name_labels[6]; // Name labels for scanner updates
static lv_obj_t* latency_labels[6]; // Latency labels for scanner updates

// Detail window objects
static lv_obj_t* detail_window = nullptr;
static lv_obj_t* detail_label = nullptr;
static bool detail_window_open = false;

// Function to close detail window
void closeDetailWindow() {
  if (detail_window_open && detail_window) {
    lv_obj_del(detail_window);
    detail_window = nullptr;
    detail_label = nullptr;
    detail_window_open = false;

  }
}

// Scanner integration variables
static unsigned long last_scan_time = 0;
static const unsigned long SCAN_INTERVAL = 30000; // 60 seconds between scans
static bool scanner_initialized = false;

// Footer update variables
static unsigned long last_uptime_update = 0;
static const unsigned long UPTIME_UPDATE_INTERVAL = 500; // 1 second between updates
static unsigned long start_time = 0;
static lv_obj_t* uptime_label_ref = nullptr; // Global reference for uptime label
static lv_obj_t* footer_ref = nullptr; // Global reference for footer
static lv_obj_t* wifi_indicator_ref = nullptr; // Global reference for WiFi indicator
static bool footer_show_network = true; // Toggle state for footer
static int footer_mode = 0; // Footer mode: 0=System, 1=Network, 2=Performance, 3=Targets, 4=Uptime
static lv_obj_t* footer_line2_ref = nullptr; // Global reference for footer line 2

// Telegram chat ID discovery variables

static bool telegram_initialized = false;

// Footer long press variables
static bool footer_pressed = false;
static unsigned long footer_press_start = 0;

// Global touch filter variables
static unsigned long last_touch_time = 0;
static const unsigned long TOUCH_FILTER_MS = 500; // 500ms filter

// Function to show detail window
void showDetailWindow(int target_index) {
  if (detail_window_open || target_index < 0 || target_index >= N_TARGETS) {
    return;
  }
  
  // Create detail window (modal)
  detail_window = lv_obj_create(main_screen);
  lv_obj_set_size(detail_window, 200, 120);
  lv_obj_center(detail_window);
  lv_obj_set_style_bg_color(detail_window, lv_color_hex(0x666666), LV_PART_MAIN); // Gray background
  lv_obj_set_style_border_width(detail_window, 2, LV_PART_MAIN);
  lv_obj_set_style_border_color(detail_window, lv_color_hex(0x999999), LV_PART_MAIN);
  lv_obj_set_style_radius(detail_window, 10, LV_PART_MAIN);
  lv_obj_set_style_pad_all(detail_window, 20, LV_PART_MAIN);
  
  // Create detail label with target name
  detail_label = lv_label_create(detail_window);
  lv_label_set_text(detail_label, targets[target_index].name);
  lv_obj_set_style_text_color(detail_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_text_font(detail_label, LV_FONT_DEFAULT, LV_PART_MAIN);
  lv_obj_center(detail_label);
  
  detail_window_open = true;

}

// (Removed) scan status indicator

// Function to update footer with all modes (single line, abbreviated)
void updateFooterContent() {
  if (!uptime_label_ref) return;
  
  char line[60];
  
  switch (footer_mode) {
    case 0: // System Status - only calculate when needed
      {
        static int last_active_alerts = -1, last_targets_up = -1;
        static unsigned long last_targets_calc = 0;
        
        // Only recalculate targets every 2 seconds to save CPU
        if (millis() - last_targets_calc >= 2000) {
          int active_alerts = 0, targets_up = 0;
          for (int i = 0; i < N_TARGETS; i++) {
            if (ScanManager::getTargetStatus(i) == DOWN) active_alerts++;
            if (ScanManager::getTargetStatus(i) == UP) targets_up++;
          }
          last_active_alerts = active_alerts;
          last_targets_up = targets_up;
          last_targets_calc = millis();
        }
        
        snprintf(line, sizeof(line), "Alerting: %d | Online: %d/%d", 
                 last_active_alerts, last_targets_up, N_TARGETS);
      }
      break;
      
    case 1: // Network Info
      snprintf(line, sizeof(line), "%s | %d dBm | %ds", 
               WiFi.localIP().toString().c_str(), WiFi.RSSI(), SCAN_INTERVAL / 120000);
      break;
      
    case 2: // Performance - cache expensive calculations
      {
        static int last_cpu = 0, last_ram = 0;
        static unsigned long last_perf_calc = 0;
        
        // Only recalculate CPU/RAM every 1 second to save CPU
        if (millis() - last_perf_calc >= 1000) {
          last_cpu = (int)((millis() % 1000) / 10);
          last_ram = (int)(ESP.getFreeHeap() * 100 / ESP.getHeapSize());
          last_perf_calc = millis();
        }
        
        unsigned long uptime_seconds = (millis() - start_time) / 1000;
        unsigned long hours = uptime_seconds / 3600;
        unsigned long minutes = (uptime_seconds % 3600) / 60;
        snprintf(line, sizeof(line), "Cpu: %d%% | Ram: %d%%", 
                 last_cpu, last_ram);
      }
      break;
      
    case 3: // Target Details - reuse cached values from mode 0
      {
        static int last_targets_up = 0, last_targets_down = 0;
        static unsigned long last_targets_calc = 0;
        
        // Only recalculate targets every 2 seconds to save CPU
        if (millis() - last_targets_calc >= 2000) {
          int targets_up = 0, targets_down = 0;
          for (int i = 0; i < N_TARGETS; i++) {
            if (ScanManager::getTargetStatus(i) == UP) targets_up++;
            else if (ScanManager::getTargetStatus(i) == DOWN) targets_down++;
          }
          last_targets_up = targets_up;
          last_targets_down = targets_down;
          last_targets_calc = millis();
        }
        
        snprintf(line, sizeof(line), "UP: %d | DW: %d | %s", 
                 last_targets_up, last_targets_down, ScanManager::isActive() ? "SCAN" : "IDLE");
      }
      break;
      
    case 4: // Uptime & Status
      {
        unsigned long uptime_seconds = (millis() - start_time) / 1000;
        unsigned long hours = uptime_seconds / 3600;
        unsigned long minutes = (uptime_seconds % 3600) / 60;
        snprintf(line, sizeof(line), "Up: %02lu:%02lu | Rom: %dKB | Nx: %ds", 
                 hours, minutes, ESP.getFreeHeap() / 1024,
                 (SCAN_INTERVAL - (millis() - last_scan_time)) / 120000);
      }
      break;
  }
  
  lv_label_set_text(uptime_label_ref, line);
}
static const unsigned long FOOTER_LONG_PRESS_TIME = 500; // press sensitivity

// Footer click callback
void footer_click_cb(lv_event_t* e) {
  TOUCH_LOGLN("[FOOTER] Callback chamado! Trocando modo...");
  
  // Cycle through all footer modes
  footer_mode = (footer_mode + 1) % 5;
  
  const char* mode_names[] = {"System", "Network", "Performance", "Targets", "Uptime"};
  TOUCH_LOGF("[FOOTER] Novo modo: %s\n", mode_names[footer_mode]);
  
  // Update footer content
  updateFooterContent();
  
  // Force display refresh
  lv_refr_now(lv_disp_get_default());
  TOUCH_LOGLN("[FOOTER] Display forçado a atualizar!");
}

void setup() {
  Serial.begin(115200);
  LOGLN("[MAIN] Iniciando Nebula Monitor v2.3...");
  
  // Inicializar ConfigManager
  Serial.println("[MAIN] Inicializando ConfigManager...");
  if (!ConfigManager::begin()) {
    Serial.println("[MAIN] ERRO: Falha ao inicializar ConfigManager!");
    Serial.println("[MAIN] Continuando com valores padrão...");
    // Continue com valores padrão
  } else {
    Serial.println("[MAIN] ConfigManager inicializado com sucesso!");
    ConfigManager::printAllConfigs();
    
    // Inicializar variáveis de debug
    DEBUG_LOGS_ENABLED = ConfigManager::isDebugLogsEnabled();
    TOUCH_LOGS_ENABLED = ConfigManager::isTouchLogsEnabled();
    ALL_LOGS_ENABLED = ConfigManager::isAllLogsEnabled();
    
    Serial.printf("[MAIN] Debug configurado: DEBUG=%s, TOUCH=%s, ALL=%s\n",
                  DEBUG_LOGS_ENABLED ? "ON" : "OFF",
                  TOUCH_LOGS_ENABLED ? "ON" : "OFF", 
                  ALL_LOGS_ENABLED ? "ON" : "OFF");
  }
  
  // Carregar targets do config.env
  Serial.println("[MAIN] Carregando targets...");
  loadTargetsFromConfig();
  Serial.printf("[MAIN] %d targets carregados com sucesso!\n", N_TARGETS);
  
  // Connect to WiFi
  LOGLN("[MAIN] Conectando ao WiFi...");
  if (!Net::connectWiFi(WIFI_SSID, WIFI_PASS)) {
    LOGLN("[MAIN] ERRO: Falha ao conectar WiFi!");
    // Continue anyway, maybe WiFi will connect later
  } else {
    LOGLN("[MAIN] WiFi conectado com sucesso!");
    Net::printInfo();
  }
  
  start_time = millis(); // Initialize start time for uptime calculation

  // Initialize display
  Serial.println("[MAIN] Inicializando display...");
  if (!DisplayManager::begin()) {
    Serial.println("[MAIN] ERRO: Falha ao inicializar display!");
    return;
  }
  Serial.println("[MAIN] Display inicializado com sucesso!");

  // Load LED configuration
  LED_PIN_R = ConfigManager::getLedPinR();
  LED_PIN_G = ConfigManager::getLedPinG();
  LED_PIN_B = ConfigManager::getLedPinB();
  LED_ACTIVE_HIGH = ConfigManager::isLedActiveHigh();
  LEDC_FREQ = ConfigManager::getLedPwmFreq();
  LEDC_RES_BITS = ConfigManager::getLedPwmResBits();
  LED_BRIGHT_R = ConfigManager::getLedBrightR();
  LED_BRIGHT_G = ConfigManager::getLedBrightG();
  LED_BRIGHT_B = ConfigManager::getLedBrightB();

  // Initialize RGB status LED PWM
  ledcSetup(LEDC_CHANNEL_R, LEDC_FREQ, LEDC_RES_BITS);
  ledcSetup(LEDC_CHANNEL_G, LEDC_FREQ, LEDC_RES_BITS);
  ledcSetup(LEDC_CHANNEL_B, LEDC_FREQ, LEDC_RES_BITS);
  ledcAttachPin(LED_PIN_R, LEDC_CHANNEL_R);
  ledcAttachPin(LED_PIN_G, LEDC_CHANNEL_G);
  ledcAttachPin(LED_PIN_B, LEDC_CHANNEL_B);
  setStatusLed(false, false, false); // start off

  // Initialize touch
  if (!Touch::beginHSPI()) {
    LOGLN("[MAIN] ERRO: Falha ao inicializar touch!");
    return;
  }
  LOGLN("[MAIN] Touch inicializado com sucesso!");

  // Initialize network scanner
  if (!ScanManager::begin(targets, N_TARGETS)) {
    LOGLN("[MAIN] ERRO: Falha ao inicializar scanner!");
    return;
  }
  // Wire scan callbacks to sync UI + LED exactly at start/end
  ScanManager::setCallbacks(
    [](){
      // Scan started: set LED to blue (scan state), refresh footer immediately
      setStatusLed(false, false, true);
      updateFooterContent();
      lv_refr_now(lv_disp_get_default());
    },
    [](){
      // Scan completed: update UI list from latest results, footer and LED coherently
      for (int i = 0; i < N_TARGETS; i++) {
        Status status = ScanManager::getTargetStatus(i);
        uint16_t latency = ScanManager::getTargetLatency(i);
        // Update alert state per target immediately
        updateTelegramAlert(i, status, latency);
        if (status == UP && latency > 0) {
          char latency_text[30];
          const char* type_text = targets[i].monitor_type == HEALTH_CHECK ? "OK" : "ms";
          snprintf(latency_text, sizeof(latency_text), "%d %s", latency, type_text);
          lv_label_set_text(latency_labels[i], latency_text);
          if (latency < 500) {
            lv_obj_set_style_bg_color(status_labels[i], lv_color_hex(0xFF00FF), LV_PART_MAIN);
            lv_obj_set_style_text_color(latency_labels[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN);
            lv_obj_set_style_text_color(name_labels[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN);
          } else {
            lv_obj_set_style_bg_color(status_labels[i], lv_color_hex(0x0086ff), LV_PART_MAIN);
            lv_obj_set_style_text_color(latency_labels[i], lv_color_hex(0x000000), LV_PART_MAIN);
            lv_obj_set_style_text_color(name_labels[i], lv_color_hex(0x000000), LV_PART_MAIN);
          }
        } else {
          const char* down_text = targets[i].monitor_type == HEALTH_CHECK ? "HEALTH FAIL" : "DOWN";
          lv_label_set_text(latency_labels[i], down_text);
          lv_obj_set_style_bg_color(status_labels[i], lv_color_hex(0x00FFFF), LV_PART_MAIN);
          lv_obj_set_style_text_color(latency_labels[i], lv_color_hex(0x000000), LV_PART_MAIN);
          lv_obj_set_style_text_color(name_labels[i], lv_color_hex(0x000000), LV_PART_MAIN);
        }
        lv_obj_invalidate(status_labels[i]);
      }
      // Footer reflects latest status
      updateFooterContent();

      // LED based on alerts: any DOWN? -> RED; else if Telegram sending or scanning -> BLUE; else GREEN
      bool anyDown = false;
      for (int i = 0; i < N_TARGETS; i++) {
        Status s = ScanManager::getTargetStatus(i);
        if (s == DOWN) { anyDown = true; }
      }
      if (anyDown) {
        setStatusLed(true, false, false); // RED
      } else if (TelegramAlerts::isSendingMessage() || ScanManager::isActive()) {
        setStatusLed(false, false, true); // BLUE
      } else {
        setStatusLed(false, true, false);  // GREEN
      }

      // Force an immediate display refresh
      lv_refr_now(lv_disp_get_default());
      // Mark display update time to align footer countdown
      last_scan_time = millis();
    },
    [](int idx, Status status, uint16_t latency){
      // Atualiza somente o target afetado
      if (idx >= 0 && idx < N_TARGETS) {
        updateTelegramAlert(idx, status, latency);
        if (status == UP && latency > 0) {
          char latency_text[30];
          const char* type_text = targets[idx].monitor_type == HEALTH_CHECK ? "OK" : "ms";
          snprintf(latency_text, sizeof(latency_text), "%d %s", latency, type_text);
          lv_label_set_text(latency_labels[idx], latency_text);
          if (latency < 500) {
            lv_obj_set_style_bg_color(status_labels[idx], lv_color_hex(0xFF00FF), LV_PART_MAIN);
            lv_obj_set_style_text_color(latency_labels[idx], lv_color_hex(0xFFFFFF), LV_PART_MAIN);
            lv_obj_set_style_text_color(name_labels[idx], lv_color_hex(0xFFFFFF), LV_PART_MAIN);
          } else {
            // Laranja (cor trabalhada via paleta invertida → azul aqui): mantemos cor de "slow"
            lv_obj_set_style_bg_color(status_labels[idx], lv_color_hex(0x0086ff), LV_PART_MAIN);
            lv_obj_set_style_text_color(latency_labels[idx], lv_color_hex(0x000000), LV_PART_MAIN);
            lv_obj_set_style_text_color(name_labels[idx], lv_color_hex(0x000000), LV_PART_MAIN);
          }
        } else {
          const char* down_text = targets[idx].monitor_type == HEALTH_CHECK ? "HEALTH FAIL" : "DOWN";
          lv_label_set_text(latency_labels[idx], down_text);
          lv_obj_set_style_bg_color(status_labels[idx], lv_color_hex(0x00FFFF), LV_PART_MAIN);
          lv_obj_set_style_text_color(latency_labels[idx], lv_color_hex(0x000000), LV_PART_MAIN);
          lv_obj_set_style_text_color(name_labels[idx], lv_color_hex(0x000000), LV_PART_MAIN);
        }
        lv_obj_invalidate(status_labels[idx]);

        // Atualiza LED por target: se algum DOWN até aqui, LED vermelho; se scan ativo, azul; senão verde
        bool anyDown = false;
        for (int i = 0; i < N_TARGETS; i++) {
          if (ScanManager::getTargetStatus(i) == DOWN) { anyDown = true; break; }
        }
        if (anyDown) setStatusLed(true, false, false);
        else if (TelegramAlerts::isSendingMessage() || ScanManager::isActive()) setStatusLed(false, false, true);
        else setStatusLed(false, true, false);

        // Refresh parcial/imediato
        lv_refr_now(lv_disp_get_default());
      }
    }
  );
  scanner_initialized = true;
  LOGLN("[MAIN] Scanner inicializado com sucesso!");

  // Initialize Telegram alerts
  if (initTelegramAlerts()) {
    LOGLN("[MAIN] Sistema de alertas Telegram inicializado!");
    telegram_initialized = true;
    // Enviar mensagem de inicialização
    delay(2000); // Aguardar um pouco
    sendTestTelegramAlert();
  } else {
    LOGLN("[MAIN] Sistema de alertas Telegram não inicializado (configuração necessária)");
    telegram_initialized = false;
  }
  
  // Verificar status do Telegram
  LOGF("[MAIN] Status do Telegram: %s\n", telegram_initialized ? "INICIALIZADO" : "NÃO INICIALIZADO");

  // Initialize LVGL screen
  main_screen = lv_scr_act();
  lv_obj_set_style_bg_color(main_screen, lv_color_hex(0x000000), LV_PART_MAIN); // Preto → aparecerá branco

  // Create title bar
  lv_obj_t* title_bar = lv_obj_create(main_screen);
  lv_obj_set_size(title_bar, 240, 40);
  lv_obj_set_pos(title_bar, 0, 0);
  lv_obj_set_style_bg_color(title_bar, lv_color_hex(0x2d2d2d), LV_PART_MAIN);
  lv_obj_set_style_border_width(title_bar, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(title_bar, 0, LV_PART_MAIN);

  // Create title label
  title_label = lv_label_create(title_bar);
  lv_label_set_text(title_label, "Nebula Monitor v2.3");
  lv_obj_set_style_text_color(title_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_text_font(title_label, LV_FONT_DEFAULT, LV_PART_MAIN);
  lv_obj_center(title_label);

  // No more buttons - clean interface with just the list!
  Serial.println("[MAIN] Interface limpa - sem botões, só lista!");

  // Create main form container with flex layout - increased height
  lv_obj_t* main_form = lv_obj_create(main_screen);
  lv_obj_set_size(main_form, 220, 220); // Increased height for more list space
  lv_obj_set_pos(main_form, 10, 50);
  lv_obj_set_style_bg_color(main_form, lv_color_hex(0x222222), LV_PART_MAIN);
  lv_obj_set_style_border_width(main_form, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(main_form, 8, LV_PART_MAIN);
  lv_obj_set_style_pad_all(main_form, 10, LV_PART_MAIN);
  
  // Enable flex layout for the form
  lv_obj_set_flex_flow(main_form, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(main_form, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  
            // Create status items with flex layout
      for (int i = 0; i < N_TARGETS; i++) {
        // Status item container - flex item
        lv_obj_t* status_item = lv_obj_create(main_form);
        lv_obj_set_size(status_item, 200, 24); // Fixed width, flexible height
        lv_obj_set_style_bg_color(status_item, lv_color_hex(0x111111), LV_PART_MAIN);
        lv_obj_set_style_border_width(status_item, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(status_item, 6, LV_PART_MAIN);
        lv_obj_set_style_pad_all(status_item, 4, LV_PART_MAIN);
        lv_obj_set_style_pad_bottom(status_item, 2, LV_PART_MAIN); // Space between items
        
        // Enable flex layout for the item (horizontal)
        lv_obj_set_flex_flow(status_item, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(status_item, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        // Target name label (left side)
        lv_obj_t* name_label = lv_label_create(status_item);
        lv_label_set_text(name_label, targets[i].name);
        lv_obj_set_style_text_color(name_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_text_font(name_label, LV_FONT_DEFAULT, LV_PART_MAIN);

        // Latency label (right side) - store reference for updates
        lv_obj_t* latency_label = lv_label_create(status_item);
        lv_label_set_text(latency_label, "--- ms");
        lv_obj_set_style_text_color(latency_label, lv_color_hex(0xCCCCCC), LV_PART_MAIN);
        lv_obj_set_style_text_font(latency_label, LV_FONT_DEFAULT, LV_PART_MAIN);
        
        // Store references for scanner updates
        status_labels[i] = status_item;
        name_labels[i] = name_label;
        latency_labels[i] = latency_label;
      }

  // Create footer with flex layout - closer to the list
  lv_obj_t* footer = lv_obj_create(main_screen);
  lv_obj_set_size(footer, 220, 32); // Single line height
  lv_obj_set_pos(footer, 10, 260); // Back to original position
  lv_obj_set_style_bg_color(footer, lv_color_hex(0x333333), LV_PART_MAIN);
  lv_obj_set_style_border_width(footer, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(footer, 6, LV_PART_MAIN);
  lv_obj_set_style_pad_all(footer, 6, LV_PART_MAIN);
  
  // Make footer clickable
  lv_obj_add_event_cb(footer, footer_click_cb, LV_EVENT_CLICKED, nullptr);
  
  // Enable flex layout for footer (horizontal) - centered
  lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  // Main footer text (single line with abbreviated info)
  lv_obj_t* footer_main_text = lv_label_create(footer);
  lv_label_set_text(footer_main_text, "Sys: OK | Alt: 0 | 6/6 UP");
  lv_obj_set_style_text_color(footer_main_text, lv_color_hex(0xCCCCCC), LV_PART_MAIN);
  lv_obj_set_style_text_font(footer_main_text, LV_FONT_DEFAULT, LV_PART_MAIN);
  lv_obj_set_width(footer_main_text, LV_PCT(100));
  lv_obj_set_style_text_align(footer_main_text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  
  // Store references for dynamic updates
  uptime_label_ref = footer_main_text;
  footer_ref = footer;

  
  
  // Start scanning after everything is initialized
  ScanManager::startScanning();
  Serial.println("[MAIN] Scanner iniciado!");
  
  Serial.println("[MAIN] Setup completo! Interface pronta com footer!");
}

void loop() {
  // Handle LVGL tasks
  lv_timer_handler();
  
  // Small delay to give touch more processing time
  delay(10);

  // Check WiFi connection and reconnect if needed
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[MAIN] WiFi desconectado, tentando reconectar...");
    if (Net::connectWiFi(WIFI_SSID, WIFI_PASS, 10000)) { // 10 second timeout
      Serial.println("[MAIN] WiFi reconectado!");
      Net::printInfo();
    } else {
      Serial.println("[MAIN] Falha na reconexão WiFi");
    }
  }

  // Real-time LED priority while in loop: DOWN -> RED, else TELEGRAM/SCAN -> BLUE, else GREEN
  bool anyDownRealtime = false;
  for (int i = 0; i < N_TARGETS; i++) {
    if (ScanManager::getTargetStatus(i) == DOWN) { anyDownRealtime = true; break; }
  }
  if (anyDownRealtime) setStatusLed(true, false, false);
  else if (TelegramAlerts::isSendingMessage() || ScanManager::isActive()) setStatusLed(false, false, true);
  else setStatusLed(false, true, false);

  // Wi-Fi fail-safe: blink RED when disconnected (overrides other states)
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long lastBlink = 0;
    static bool on = false;
    if (millis() - lastBlink >= 500) { // 500ms blink rate
      on = !on;
      lastBlink = millis();
      setStatusLed(on, false, false);
    }
  }

  // Handle network scanning
  if (scanner_initialized) {
    // Update scanner state machine
    ScanManager::update();
    

    
    // Check if we need to update display
    if (millis() - last_scan_time >= SCAN_INTERVAL) {
      DEBUG_LOGLN("[SCANNER] Atualizando display...");
      
      // Update all targets from scanner data
      for (int i = 0; i < N_TARGETS; i++) {
        // Get scan result from scanner
        Status status = ScanManager::getTargetStatus(i);
        uint16_t latency = ScanManager::getTargetLatency(i);
        
        // Debug: mostrar status e latência
        DEBUG_LOGF("[FRONTEND] Target %d: Status=%d, Latency=%d\n", i, status, latency);
        
        // Atualizar sistema de alertas
        DEBUG_LOGF("[MAIN] Chamando updateTelegramAlert para target %d (status=%d, latency=%d)\n", i, status, latency);
        updateTelegramAlert(i, status, latency);
        
        if (status == UP && latency > 0) {
          char latency_text[30];
          const char* type_text = targets[i].monitor_type == HEALTH_CHECK ? "OK" : "ms";
          snprintf(latency_text, sizeof(latency_text), "%d %s", latency, type_text);
          lv_label_set_text(latency_labels[i], latency_text);
          
          // Determine color based on latency threshold
          if (latency < 500) {
            // Green for good latency (< 500ms)
            lv_obj_set_style_bg_color(status_labels[i], lv_color_hex(0xFF00FF), LV_PART_MAIN); // Verde (inverso)
            lv_obj_set_style_text_color(latency_labels[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN); // Texto branco
            lv_obj_set_style_text_color(name_labels[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN); // Nome branco
            Serial.printf("[SCANNER] %s: %d %s (UP - Verde)\n", targets[i].name, latency, type_text);
          } else {
            // Blue for high latency (>= 500ms)
            lv_obj_set_style_bg_color(status_labels[i], lv_color_hex(0x0086ff), LV_PART_MAIN); // Azul (cor diferente para não confundir)
            lv_obj_set_style_text_color(latency_labels[i], lv_color_hex(0x000000), LV_PART_MAIN); // Texto preto
            lv_obj_set_style_text_color(name_labels[i], lv_color_hex(0x000000), LV_PART_MAIN); // Nome preto
            Serial.printf("[SCANNER] %s: %d %s (UP - Azul)\n", targets[i].name, latency, type_text);
          }
        } else {
          const char* down_text = targets[i].monitor_type == HEALTH_CHECK ? "HEALTH FAIL" : "DOWN";
          lv_label_set_text(latency_labels[i], down_text);
          
          // Update status color (red for failure) - usando cor inversa para display invertido
          lv_obj_set_style_bg_color(status_labels[i], lv_color_hex(0x00FFFF), LV_PART_MAIN);
          
          // Texto preto para fundo vermelho (melhor legibilidade) - usando cor inversa para display invertido
          lv_obj_set_style_text_color(latency_labels[i], lv_color_hex(0x000000), LV_PART_MAIN);
          lv_obj_set_style_text_color(name_labels[i], lv_color_hex(0x000000), LV_PART_MAIN);
          
          Serial.printf("[SCANNER] %s: %s\n", targets[i].name, down_text);
        }
        
        // Force refresh of the item
        lv_obj_invalidate(status_labels[i]);
      }
      
      // Force display refresh
      lv_refr_now(lv_disp_get_default());
      
      last_scan_time = millis();
      Serial.println("[SCANNER] Display atualizado!");
    }
    
    // Update footer based on current mode - only when footer is visible
    if (millis() - last_uptime_update >= UPTIME_UPDATE_INTERVAL) {
      // Only update footer content, don't do heavy calculations
      updateFooterContent();
      last_uptime_update = millis();
    }
  }


  
  // Verificar status do Telegram periodicamente (debug)
  static unsigned long last_telegram_check = 0;
  if (millis() - last_telegram_check >= 240000) { // A cada 10 segundos
    Serial.printf("[MAIN] Status Telegram: %s\n", telegram_initialized ? "ATIVO" : "INATIVO");
    last_telegram_check = millis();
  }

  // Handle touch input with global filter
  if (Touch::touched()) {
    // Apply global touch filter (500ms)
    unsigned long current_time = millis();
    if (current_time - last_touch_time < TOUCH_FILTER_MS) {
      return; // Ignore touch if within filter period
    }
    last_touch_time = current_time;
    
    int16_t raw_x, raw_y, z;
    Touch::readRaw(raw_x, raw_y, z);
    
    int screen_x, screen_y;
    Touch::mapRawToScreen(raw_x, raw_y, screen_x, screen_y);
    
    TOUCH_LOGF("[TOUCH] Touch detectado em (%d, %d) - Filtro aplicado\n", screen_x, screen_y);
    
    // Check if footer was touched
    if (footer_ref) {
      lv_area_t footer_area;
      lv_obj_get_coords(footer_ref, &footer_area);
      
      TOUCH_LOGF("[TOUCH] Footer area: (%d,%d) to (%d,%d)\n", 
                   footer_area.x1, footer_area.y1, footer_area.x2, footer_area.y2);
      
      if (screen_x >= footer_area.x1 && screen_x < footer_area.x2 && 
          screen_y >= footer_area.y1 && screen_y < footer_area.y2) {
        
        // Footer touched - start long press detection
        if (!footer_pressed) {
          footer_pressed = true;
          footer_press_start = millis();
          TOUCH_LOGLN("[TOUCH] FOOTER PRESSIONADO! Iniciando contagem de 2s...");
          
          // // Visual feedback - change footer color to indicate press
          // lv_obj_set_style_bg_color(footer_ref, lv_color_hex(0x555555), LV_PART_MAIN);
          // lv_refr_now(lv_disp_get_default());
        }
        
        // Check if long press time reached
        if (footer_pressed && (millis() - footer_press_start >= FOOTER_LONG_PRESS_TIME)) {
          TOUCH_LOGLN("[TOUCH] PRESSÃO LONGA ATINGIDA! Executando toggle...");
          
          // Execute toggle
          footer_click_cb(nullptr);
          
          // Reset press state
          footer_pressed = false;
          footer_press_start = 0;
          
          // // Restore original footer color
          // lv_obj_set_style_bg_color(footer_ref, lv_color_hex(0x333333), LV_PART_MAIN);
          // lv_refr_now(lv_disp_get_default());
          
          TOUCH_LOGLN("[TOUCH] Toggle executado e footer restaurado!");
          return; // Exit early since footer was toggled
        }
      } else {
        // Touch outside footer - reset press state
        if (footer_pressed) {
          footer_pressed = false;
          footer_press_start = 0;
          TOUCH_LOGLN("[TOUCH] Touch fora do footer - resetando pressão!");
          
          // Restore original footer color
          lv_obj_set_style_bg_color(footer_ref, lv_color_hex(0x333333), LV_PART_MAIN);
          lv_refr_now(lv_disp_get_default());
        }
      }
    }
    
    // Check if detail window is open and was touched
    if (detail_window_open && detail_window) {
      lv_area_t window_area;
      lv_obj_get_coords(detail_window, &window_area);
      
      if (screen_x >= window_area.x1 && screen_x < window_area.x2 && 
          screen_y >= window_area.y1 && screen_y < window_area.y2) {
        // Detail window was touched - close it
        closeDetailWindow();
        TOUCH_LOGLN("[TOUCH] Janela de detalhes fechada por toque");
        return; // Exit early since window was closed
      }
    }
    
    // Check which status item was touched
    for (int i = 0; i < N_TARGETS; i++) {
      lv_area_t area;
      lv_obj_get_coords(status_labels[i], &area);
      
      if (screen_x >= area.x1 && screen_x < area.x2 && 
          screen_y >= area.y1 && screen_y < area.y2) {
        
        TOUCH_LOGF("[TOUCH] Status item %d (%s) tocado\n", 
                     i, targets[i].name);
        
        // Show detail window
        showDetailWindow(i);
        break;
      }
    }
  }

  // Small delay for stability
  delay(10);
}
