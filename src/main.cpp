#include <Arduino.h>
#include <WiFi.h>
#include "config.hpp"
#include "display.hpp"
#include "touch.hpp"
#include "net.hpp"
#include "scan.hpp"
#include <lvgl.h>

// Network targets
const Target targets[] = {
  {"Proxmox HV", "http://192.168.1.128:8006/", UNKNOWN, 0},
  {"Polaris API", "https://endangered-musician-bolt-berlin.trycloudflare.com", UNKNOWN, 0},
  {"Polaris INT", "https://ebfc52323306.ngrok-free.app", UNKNOWN, 0},
  {"Polaris WEB", "https://tech-tweakers.github.io/polaris-v2-web/", UNKNOWN, 0},
  {"Router #1", "http://192.168.1.1", UNKNOWN, 0},
  {"Router #2", "https://192.168.1.172", UNKNOWN, 0}
};

const int N_TARGETS = sizeof(targets) / sizeof(targets[0]);

// LVGL objects
static lv_obj_t* main_screen;
static lv_obj_t* title_label;
static lv_obj_t* tile_containers[6]; // Fixed size for now
static lv_obj_t* status_labels[6]; // Status items for scanner updates
static lv_obj_t* name_labels[6]; // Name labels for scanner updates
static lv_obj_t* latency_labels[6]; // Latency labels for scanner updates

// Scanner integration variables
static unsigned long last_scan_time = 0;
static const unsigned long SCAN_INTERVAL = 5000; // 5 seconds between scans
static bool scanner_initialized = false;

// Footer update variables
static unsigned long last_uptime_update = 0;
static const unsigned long UPTIME_UPDATE_INTERVAL = 500; // 1 second between updates
static unsigned long start_time = 0;
static lv_obj_t* uptime_label_ref = nullptr; // Global reference for uptime label
static lv_obj_t* footer_ref = nullptr; // Global reference for footer
static bool footer_show_network = true; // Toggle state for footer

// Footer long press variables
static bool footer_pressed = false;
static unsigned long footer_press_start = 0;
static const unsigned long FOOTER_LONG_PRESS_TIME = 500; // 2 seconds for long press

// Footer click callback
void footer_click_cb(lv_event_t* e) {
  Serial.println("[FOOTER] Callback chamado! Trocando estado...");
  
  footer_show_network = !footer_show_network; // Toggle state
  
  Serial.printf("[FOOTER] Novo estado: %s\n", footer_show_network ? "Rede" : "Status");
  
  if (footer_show_network) {
    // Show network info
    char network_info[50];
    snprintf(network_info, sizeof(network_info), "WiFi: OK | IP: %s", WiFi.localIP().toString().c_str());
    Serial.printf("[FOOTER] Mostrando rede: %s\n", network_info);
    if (uptime_label_ref) {
      lv_label_set_text(uptime_label_ref, network_info);
      Serial.println("[FOOTER] Texto atualizado para rede!");
    } else {
      Serial.println("[FOOTER] ERRO: uptime_label_ref é NULL!");
    }
  } else {
    // Show uptime and status
    unsigned long uptime_seconds = (millis() - start_time) / 1000;
    unsigned long hours = uptime_seconds / 3600;
    unsigned long minutes = (uptime_seconds % 3600) / 60;
    
    // Count targets status
    int targets_ok = 0;
    int targets_fail = 0;
    for (int i = 0; i < N_TARGETS; i++) {
      if (ScanManager::getTargetStatus(i) == UP) {
        targets_ok++;
      } else {
        targets_fail++;
      }
    }
    
    char status_info[50];
    if (hours > 0) {
      snprintf(status_info, sizeof(status_info), "UP: %02lu:%02lu | OK:%d FAIL:%d", hours, minutes, targets_ok, targets_fail);
    } else {
      snprintf(status_info, sizeof(status_info), "UP: %02lu:%02lu | OK:%d FAIL:%d", minutes, uptime_seconds % 60, targets_ok, targets_fail);
    }
    
    Serial.printf("[FOOTER] Mostrando status: %s\n", status_info);
    if (uptime_label_ref) {
      lv_label_set_text(uptime_label_ref, status_info);
      Serial.println("[FOOTER] Texto atualizado para status!");
    } else {
      Serial.println("[FOOTER] ERRO: uptime_label_ref é NULL!");
    }
  }
  
  // Force display refresh
  lv_refr_now(lv_disp_get_default());
  Serial.println("[FOOTER] Display forçado a atualizar!");
}

void setup() {
  Serial.begin(115200);
  Serial.println("[MAIN] Iniciando Nebula Monitor v3.0...");
  
  // Connect to WiFi
  Serial.println("[MAIN] Conectando ao WiFi...");
  if (!Net::connectWiFi(WIFI_SSID, WIFI_PASS)) {
    Serial.println("[MAIN] ERRO: Falha ao conectar WiFi!");
    // Continue anyway, maybe WiFi will connect later
  } else {
    Serial.println("[MAIN] WiFi conectado com sucesso!");
    Net::printInfo();
  }
  
  start_time = millis(); // Initialize start time for uptime calculation

  // Initialize display
  if (!DisplayManager::begin()) {
    Serial.println("[MAIN] ERRO: Falha ao inicializar display!");
    return;
  }
  Serial.println("[MAIN] Display inicializado com sucesso!");

  // Initialize touch
  if (!Touch::beginHSPI()) {
    Serial.println("[MAIN] ERRO: Falha ao inicializar touch!");
    return;
  }
  Serial.println("[MAIN] Touch inicializado com sucesso!");

  // Initialize network scanner
  if (!ScanManager::begin(targets, N_TARGETS)) {
    Serial.println("[MAIN] ERRO: Falha ao inicializar scanner!");
    return;
  }
  ScanManager::startScanning(); // Start the scanner
  scanner_initialized = true;
  Serial.println("[MAIN] Scanner inicializado com sucesso!");

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
  lv_label_set_text(title_label, "Nebula Monitor v2.0");
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
  lv_obj_set_size(footer, 220, 32); // Reduced height for smaller appearance
  lv_obj_set_pos(footer, 10, 260); // Reduced margin
  lv_obj_set_style_bg_color(footer, lv_color_hex(0x333333), LV_PART_MAIN);
  lv_obj_set_style_border_width(footer, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(footer, 6, LV_PART_MAIN);
  lv_obj_set_style_pad_all(footer, 6, LV_PART_MAIN); // Reduced padding for smaller appearance
  
  // Make footer clickable
  lv_obj_add_event_cb(footer, footer_click_cb, LV_EVENT_CLICKED, nullptr);
  
  // Enable flex layout for footer (horizontal) - left aligned
  lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  // WiFi status indicator
  lv_obj_t* wifi_indicator = lv_obj_create(footer);
  lv_obj_set_size(wifi_indicator, 10, 10); // Smaller indicator
  lv_obj_set_style_bg_color(wifi_indicator, lv_color_hex(0xFF00FF), LV_PART_MAIN); // Green when connected - cor inversa para display invertido
  lv_obj_set_style_radius(wifi_indicator, 5, LV_PART_MAIN);
  lv_obj_set_style_border_width(wifi_indicator, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_right(wifi_indicator, 6, LV_PART_MAIN); // Space between indicator and text

  // Main footer text (will toggle between network info and uptime/status)
  lv_obj_t* footer_main_text = lv_label_create(footer);
  lv_label_set_text(footer_main_text, "WiFi: OK | IP: 192.168.1.162");
  lv_obj_set_style_text_color(footer_main_text, lv_color_hex(0xCCCCCC), LV_PART_MAIN);
  lv_obj_set_style_text_font(footer_main_text, LV_FONT_DEFAULT, LV_PART_MAIN); // Default font
  lv_obj_set_style_text_align(footer_main_text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN); // Left aligned text
  
  // Store references for dynamic updates
  uptime_label_ref = footer_main_text;
  footer_ref = footer;

  Serial.println("[MAIN] Setup completo! Interface pronta com footer!");
}

void loop() {
  // Handle LVGL tasks
  lv_timer_handler();

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

  // Handle network scanning
  if (scanner_initialized) {
    // Update scanner state machine
    ScanManager::update();
    
    // Check if we need to update display
    if (millis() - last_scan_time >= SCAN_INTERVAL) {
      Serial.println("[SCANNER] Atualizando display...");
      
      // Update all targets from scanner data
      for (int i = 0; i < N_TARGETS; i++) {
        // Get scan result from scanner
        Status status = ScanManager::getTargetStatus(i);
        uint16_t latency = ScanManager::getTargetLatency(i);
        
        // Debug: mostrar status e latência
        Serial.printf("[FRONTEND] Target %d: Status=%d, Latency=%d\n", i, status, latency);
        
        if (status == UP && latency > 0) {
          char latency_text[20];
          snprintf(latency_text, sizeof(latency_text), "%d ms", latency);
          lv_label_set_text(latency_labels[i], latency_text);
          
          // Determine color based on latency threshold
          if (latency < 500) {
            // Green for good latency (< 1000ms)
            lv_obj_set_style_bg_color(status_labels[i], lv_color_hex(0xFF00FF), LV_PART_MAIN); // Verde (inverso)
            lv_obj_set_style_text_color(latency_labels[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN); // Texto preto
            lv_obj_set_style_text_color(name_labels[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN); // Nome preto
            Serial.printf("[SCANNER] %s: %d ms (UP - Verde)\n", targets[i].name, latency);
          } else {
            // Orange for high latency (>= 1000ms)
            lv_obj_set_style_bg_color(status_labels[i], lv_color_hex(0x0086ff), LV_PART_MAIN); // Laranja (cor diferente para não confundir)
            lv_obj_set_style_text_color(latency_labels[i], lv_color_hex(0x000000), LV_PART_MAIN); // Texto branco
            lv_obj_set_style_text_color(name_labels[i], lv_color_hex(0x000000), LV_PART_MAIN); // Nome branco
            Serial.printf("[SCANNER] %s: %d ms (UP - Laranja)\n", targets[i].name, latency);
          }
        } else {
          lv_label_set_text(latency_labels[i], "DOWN");
          
          // Update status color (red for failure) - usando cor inversa para display invertido
          lv_obj_set_style_bg_color(status_labels[i], lv_color_hex(0x00FFFF), LV_PART_MAIN);
          
          // Texto branco para fundo vermelho (melhor legibilidade) - usando cor inversa para display invertido
          lv_obj_set_style_text_color(latency_labels[i], lv_color_hex(0x000000), LV_PART_MAIN);
          lv_obj_set_style_text_color(name_labels[i], lv_color_hex(0x000000), LV_PART_MAIN);
          
          Serial.printf("[SCANNER] %s: DOWN\n", targets[i].name);
        }
        
        // Force refresh of the item
        lv_obj_invalidate(status_labels[i]);
      }
      
      // Force display refresh
      lv_refr_now(lv_disp_get_default());
      
      last_scan_time = millis();
      Serial.println("[SCANNER] Display atualizado!");
    }
    
    // Update footer based on current mode
    if (millis() - last_uptime_update >= UPTIME_UPDATE_INTERVAL) {
      if (footer_show_network) {
        // Update network info
        char network_info[50];
        snprintf(network_info, sizeof(network_info), "WiFi: OK | IP: %s", WiFi.localIP().toString().c_str());
        if (uptime_label_ref) {
          lv_label_set_text(uptime_label_ref, network_info);
        }
      } else {
        // Update uptime and status info
        unsigned long uptime_seconds = (millis() - start_time) / 1000;
        unsigned long hours = uptime_seconds / 3600;
        unsigned long minutes = (uptime_seconds % 3600) / 60;
        
        // Count targets status
        int targets_ok = 0;
        int targets_fail = 0;
        for (int i = 0; i < N_TARGETS; i++) {
          if (ScanManager::getTargetStatus(i) == UP) {
            targets_ok++;
          } else {
            targets_fail++;
          }
        }
        
        char status_info[50];
        if (hours > 0) {
          snprintf(status_info, sizeof(status_info), "UP: %02lu:%02lu | OK:%d FAIL:%d", hours, minutes, targets_ok, targets_fail);
        } else {
          snprintf(status_info, sizeof(status_info), "UP: %02lu:%02lu | OK:%d FAIL:%d", minutes, uptime_seconds % 60, targets_ok, targets_fail);
        }
        
        if (uptime_label_ref) {
          lv_label_set_text(uptime_label_ref, status_info);
        }
      }
      last_uptime_update = millis();
    }
  }

  // Handle touch input
  if (Touch::touched()) {
    int16_t raw_x, raw_y, z;
    Touch::readRaw(raw_x, raw_y, z);
    
    int screen_x, screen_y;
    Touch::mapRawToScreen(raw_x, raw_y, screen_x, screen_y);
    
    Serial.printf("[TOUCH] Touch detectado em (%d, %d)\n", screen_x, screen_y);
    
    // Check if footer was touched
    if (footer_ref) {
      lv_area_t footer_area;
      lv_obj_get_coords(footer_ref, &footer_area);
      
      Serial.printf("[TOUCH] Footer area: (%d,%d) to (%d,%d)\n", 
                   footer_area.x1, footer_area.y1, footer_area.x2, footer_area.y2);
      
      if (screen_x >= footer_area.x1 && screen_x < footer_area.x2 && 
          screen_y >= footer_area.y1 && screen_y < footer_area.y2) {
        
        // Footer touched - start long press detection
        if (!footer_pressed) {
          footer_pressed = true;
          footer_press_start = millis();
          Serial.println("[TOUCH] FOOTER PRESSIONADO! Iniciando contagem de 2s...");
          
          // // Visual feedback - change footer color to indicate press
          // lv_obj_set_style_bg_color(footer_ref, lv_color_hex(0x555555), LV_PART_MAIN);
          // lv_refr_now(lv_disp_get_default());
        }
        
        // Check if long press time reached
        if (footer_pressed && (millis() - footer_press_start >= FOOTER_LONG_PRESS_TIME)) {
          Serial.println("[TOUCH] PRESSÃO LONGA ATINGIDA! Executando toggle...");
          
          // Execute toggle
          footer_click_cb(nullptr);
          
          // Reset press state
          footer_pressed = false;
          footer_press_start = 0;
          
          // // Restore original footer color
          // lv_obj_set_style_bg_color(footer_ref, lv_color_hex(0x333333), LV_PART_MAIN);
          // lv_refr_now(lv_disp_get_default());
          
          Serial.println("[TOUCH] Toggle executado e footer restaurado!");
          return; // Exit early since footer was toggled
        }
      } else {
        // Touch outside footer - reset press state
        if (footer_pressed) {
          footer_pressed = false;
          footer_press_start = 0;
          Serial.println("[TOUCH] Touch fora do footer - resetando pressão!");
          
          // Restore original footer color
          lv_obj_set_style_bg_color(footer_ref, lv_color_hex(0x333333), LV_PART_MAIN);
          lv_refr_now(lv_disp_get_default());
        }
      }
    }
    
    // Check which status item was touched
    for (int i = 0; i < N_TARGETS; i++) {
      lv_area_t area;
      lv_obj_get_coords(status_labels[i], &area);
      
      if (screen_x >= area.x1 && screen_x < area.x2 && 
          screen_y >= area.y1 && screen_y < area.y2) {
        
        // Generate random color for visual feedback
        uint32_t randomColor = random(0x100000, 0xFFFFFF);
        lv_obj_set_style_bg_color(status_labels[i], lv_color_hex(randomColor), LV_PART_MAIN);
        lv_obj_invalidate(status_labels[i]);
        
        Serial.printf("[TOUCH] Status item %d (%s) tocado - Cor: #%06X\n", 
                     i, targets[i].name, randomColor);
        break;
      }
    }
  }

  // Small delay for stability
  delay(10);
}
