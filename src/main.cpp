#include <Arduino.h>
#include <WiFi.h>
#include "config.hpp"
#include "display.hpp"
#include "touch.hpp"
#include "net.hpp"
#include "scan.hpp"
#include "telegram.hpp"
#include "sd_config_manager.hpp"
#include "ntp_manager.hpp"
#include <lvgl.h>

// FreeRTOS (ESP32) for multi-core tasks
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// LED Status States - Priority order (higher number = higher priority)
enum class LEDStatus {
  OFF = 0,           // All LEDs off
  SYSTEM_OK = 1,     // Green LED - System idle/OK
  TARGETS_DOWN = 2,  // Blue LED - Active alerts (targets down)
  SCANNING = 3,      // Red LED - Scanning in progress
  TELEGRAM = 4,      // Red LED - Telegram sending
  WIFI_DISCONNECTED = 5  // Red LED blinking - WiFi disconnected
};

// Network targets - will be loaded dynamically from config.env
static Target targets[10]; // Dynamic array for targets
static int N_TARGETS = 0;  // Number of loaded targets

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

// Current LED status
static LEDStatus current_led_status = LEDStatus::OFF;

// Determine the current LED status based on system conditions
LEDStatus determineLEDStatus() {
  // Check WiFi status first (highest priority)
  if (WiFi.status() != WL_CONNECTED) {
    return LEDStatus::WIFI_DISCONNECTED;
  }
  
  // Check if Telegram is sending (high priority)
  if (TelegramAlerts::isSendingMessage()) {
    return LEDStatus::TELEGRAM;
  }
  
  // Check if scanning is active (high priority)
  if (ScanManager::isActive()) {
    return LEDStatus::SCANNING;
  }
  
  // Check if there are active alerts (targets down)
  if (TelegramAlerts::hasActiveAlerts()) {
    return LEDStatus::TARGETS_DOWN;
  }
  
  // Check if any targets are down (realtime check)
  for (int i = 0; i < N_TARGETS; i++) {
    if (ScanManager::getTargetStatus(i) == DOWN) {
      return LEDStatus::TARGETS_DOWN;
    }
  }
  
  // System is OK
  return LEDStatus::SYSTEM_OK;
}

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

// Centralized LED update function
void updateStatusLed() {
  LEDStatus new_status = determineLEDStatus();
  
  // Only update if status changed
  if (new_status == current_led_status) {
    return;
  }
  
  current_led_status = new_status;
  
  // Set LED based on status
  switch (current_led_status) {
    case LEDStatus::OFF:
      setStatusLed(false, false, false);
      break;
      
    case LEDStatus::SYSTEM_OK:
      setStatusLed(false, true, false);  // Green LED
      break;
      
    case LEDStatus::TARGETS_DOWN:
      setStatusLed(false, false, true);  // Blue LED
      break;
      
    case LEDStatus::SCANNING:
    case LEDStatus::TELEGRAM:
      setStatusLed(true, false, false);  // Red LED
      break;
      
    case LEDStatus::WIFI_DISCONNECTED:
      // This will be handled by the blinking logic in the main loop
      setStatusLed(true, false, false);  // Red LED
      break;
  }
}


// Function to load targets from ConfigManager
void loadTargetsFromConfig() {
  N_TARGETS = ConfigManager::getTargetCount();
  Serial.printf("[MAIN] Loading %d targets from config.env\n", N_TARGETS);
  
  // Safety check - if no targets found, use default values
  if (N_TARGETS == 0) {
    Serial.println("[MAIN] WARNING: No targets found in config.env, using default values!");
    N_TARGETS = 6; // Use 6 default targets
    
    // Default targets hardcoded as fallback
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
      
      Serial.printf("[MAIN] Default target %d: %s | %s | %s | %s\n", 
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

// Telegram system initialization status
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

// Scan status is now handled by LED system and footer updates

// ------------------------
// Inter-task communication (FreeRTOS)
// ------------------------
enum ScanEventType { EV_SCAN_START, EV_SCAN_COMPLETE, EV_TARGET_UPDATE };
struct ScanEvent {
  ScanEventType type;
  int index;           // used for EV_TARGET_UPDATE
  Status status;       // used for EV_TARGET_UPDATE
  uint16_t latency_ms; // used for EV_TARGET_UPDATE
};

static QueueHandle_t scan_event_queue = nullptr; // events produced by scanner task and consumed by display task

// Task handles (optional)
static TaskHandle_t display_task_handle = nullptr;
static TaskHandle_t scanner_task_handle = nullptr;

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

// ------------------------
// Display/UI Task (Core 1) - LVGL operations only
// ------------------------
static void displayTask(void* pv) {
  for (;;) {
    // Drain scan events and update UI accordingly (LVGL must be accessed only here)
    if (scan_event_queue) {
      ScanEvent ev;
      while (xQueueReceive(scan_event_queue, &ev, 0) == pdTRUE) {
        if (ev.type == EV_SCAN_START) {
          updateStatusLed(); // Update LED status
          updateFooterContent();
          lv_refr_now(lv_disp_get_default());
        } else if (ev.type == EV_SCAN_COMPLETE) {
          // After scan complete, recompute UI + LED coherently
          bool anyDown = false;
          for (int i = 0; i < N_TARGETS; i++) {
            Status s = ScanManager::getTargetStatus(i);
            if (s == DOWN) { anyDown = true; }
            uint16_t latency = ScanManager::getTargetLatency(i);
            if (s == UP && latency > 0) {
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
          updateFooterContent();
          updateStatusLed(); // Update LED status based on current system state
          lv_refr_now(lv_disp_get_default());
          last_scan_time = millis();
        } else if (ev.type == EV_TARGET_UPDATE) {
          int idx = ev.index;
          if (idx >= 0 && idx < N_TARGETS) {
            updateTelegramAlert(idx, ev.status, ev.latency_ms, targets[idx].name);
            if (ev.status == UP && ev.latency_ms > 0) {
              char latency_text[30];
              const char* type_text = targets[idx].monitor_type == HEALTH_CHECK ? "OK" : "ms";
              snprintf(latency_text, sizeof(latency_text), "%d %s", ev.latency_ms, type_text);
              lv_label_set_text(latency_labels[idx], latency_text);
              if (ev.latency_ms < 500) {
                lv_obj_set_style_bg_color(status_labels[idx], lv_color_hex(0xFF00FF), LV_PART_MAIN);
                lv_obj_set_style_text_color(latency_labels[idx], lv_color_hex(0xFFFFFF), LV_PART_MAIN);
                lv_obj_set_style_text_color(name_labels[idx], lv_color_hex(0xFFFFFF), LV_PART_MAIN);
              } else {
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

            bool anyDown = false;
            for (int i = 0; i < N_TARGETS; i++) {
              if (ScanManager::getTargetStatus(i) == DOWN) { anyDown = true; break; }
            }
            updateStatusLed(); // Update LED status based on current system state

            lv_refr_now(lv_disp_get_default());
          }
        }
      }
    }

    // Handle LVGL tasks
    lv_timer_handler();

    // Small delay to give touch more processing time
    vTaskDelay(pdMS_TO_TICKS(10));

    // Wi-Fi fail-safe: blink Red LED when disconnected (highest priority)
    if (WiFi.status() != WL_CONNECTED) {
      static unsigned long lastBlink = 0;
      static bool on = false;
      if (millis() - lastBlink >= 500) {
        on = !on;
        lastBlink = millis();
        // Handle WiFi disconnected blinking - override normal LED logic
        if (on) {
          setStatusLed(true, false, false);  // Red LED on
        } else {
          setStatusLed(false, false, false); // All LEDs off
        }
      }
    } else {
      // Real-time LED priority: Targets down -> Blue, Telegram/Scan -> Red, else Green
      bool anyDownRealtime = false;
      for (int i = 0; i < N_TARGETS; i++) {
        if (ScanManager::getTargetStatus(i) == DOWN) { anyDownRealtime = true; break; }
      }
      updateStatusLed(); // Update LED status based on current system state
    }

    // Periodic footer updates
    if (millis() - last_uptime_update >= UPTIME_UPDATE_INTERVAL) {
      updateFooterContent();
      last_uptime_update = millis();
    }

    // Handle touch input with global filter
    if (Touch::touched()) {
      unsigned long current_time = millis();
      if (current_time - last_touch_time < TOUCH_FILTER_MS) {
        continue;
      }
      last_touch_time = current_time;

      int16_t raw_x, raw_y, z;
      Touch::readRaw(raw_x, raw_y, z);

      int screen_x, screen_y;
      Touch::mapRawToScreen(raw_x, raw_y, screen_x, screen_y);

      TOUCH_LOGF("[TOUCH] Touch detectado em (%d, %d) - Filtro aplicado\n", screen_x, screen_y);

      // If a detail window is open, close it on ANY tap. If tapped inside the
      // window, consume the event (do not open another). If tapped outside,
      // close it and continue processing (may open another window).
      if (detail_window_open && detail_window) {
        lv_area_t window_area;
        lv_obj_get_coords(detail_window, &window_area);
        bool inside = (screen_x >= window_area.x1 && screen_x < window_area.x2 &&
                       screen_y >= window_area.y1 && screen_y < window_area.y2);
        closeDetailWindow();
        if (inside) {
          TOUCH_LOGLN("[TOUCH] Janela de detalhes fechada por toque interno");
          continue;
        } else {
          TOUCH_LOGLN("[TOUCH] Janela de detalhes fechada por toque externo");
        }
      }

      if (footer_ref) {
        lv_area_t footer_area;
        lv_obj_get_coords(footer_ref, &footer_area);

        TOUCH_LOGF("[TOUCH] Footer area: (%d,%d) to (%d,%d)\n",
                    footer_area.x1, footer_area.y1, footer_area.x2, footer_area.y2);

        if (screen_x >= footer_area.x1 && screen_x < footer_area.x2 &&
            screen_y >= footer_area.y1 && screen_y < footer_area.y2) {
          if (!footer_pressed) {
            footer_pressed = true;
            footer_press_start = millis();
            TOUCH_LOGLN("[TOUCH] FOOTER PRESSIONADO! Iniciando contagem de 2s...");
          }

          if (footer_pressed && (millis() - footer_press_start >= FOOTER_LONG_PRESS_TIME)) {
            TOUCH_LOGLN("[TOUCH] PRESSÃO LONGA ATINGIDA! Executando toggle...");
            footer_click_cb(nullptr);
            footer_pressed = false;
            footer_press_start = 0;
            TOUCH_LOGLN("[TOUCH] Toggle executado e footer restaurado!");
            continue;
          }
        } else {
          if (footer_pressed) {
            footer_pressed = false;
            footer_press_start = 0;
            TOUCH_LOGLN("[TOUCH] Touch fora do footer - resetando pressão!");
            lv_obj_set_style_bg_color(footer_ref, lv_color_hex(0x333333), LV_PART_MAIN);
            lv_refr_now(lv_disp_get_default());
          }
        }
      }

      // Detail window touch handling already processed above

      for (int i = 0; i < N_TARGETS; i++) {
        lv_area_t area;
        lv_obj_get_coords(status_labels[i], &area);
        if (screen_x >= area.x1 && screen_x < area.x2 &&
            screen_y >= area.y1 && screen_y < area.y2) {
          TOUCH_LOGF("[TOUCH] Status item %d (%s) tocado\n",
                      i, targets[i].name);
          showDetailWindow(i);
          break;
        }
      }
    }
  }
}

// ------------------------
// Scanner Task (Core 0) - Network operations only
// ------------------------
static void scannerTask(void* pv) {
  for (;;) {
    // Check WiFi connection and reconnect if needed (keep networking on core 0)
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[SCAN-TASK] WiFi disconnected, trying to reconnect...");
      if (Net::connectWiFi(WIFI_SSID, WIFI_PASS, 10000)) {
        Serial.println("[SCAN-TASK] WiFi reconnected!");
        Net::printInfo();
      } else {
        Serial.println("[SCAN-TASK] WiFi reconnection failed");
      }
    }

    ScanManager::update();

    // Adaptive yield: when scanning, yield briefly; when idle, relax more
    if (ScanManager::isActive()) {
      vTaskDelay(pdMS_TO_TICKS(2));
    } else {
      vTaskDelay(pdMS_TO_TICKS(20));
    }
  }
}

void setup() {
  Serial.begin(115200);
  LOGLN("[MAIN] Starting Nebula Monitor v2.3...");
  
  // Initialize ConfigManager (reads from SPIFFS - no NTP yet)
  Serial.println("[MAIN] Initializing ConfigManager...");
  if (!ConfigManager::begin()) {
    Serial.println("[MAIN] ERROR: Failed to initialize ConfigManager!");
    Serial.println("[MAIN] Continuing with default values...");
    // Continue with default values
  } else {
    Serial.println("[MAIN] ConfigManager initialized successfully!");
    ConfigManager::printAllConfigs();
    
    // Initialize debug variables
    DEBUG_LOGS_ENABLED = ConfigManager::isDebugLogsEnabled();
    TOUCH_LOGS_ENABLED = ConfigManager::isTouchLogsEnabled();
    ALL_LOGS_ENABLED = ConfigManager::isAllLogsEnabled();
    
    Serial.printf("[MAIN] Debug configured: DEBUG=%s, TOUCH=%s, ALL=%s\n",
                  DEBUG_LOGS_ENABLED ? "ON" : "OFF",
                  TOUCH_LOGS_ENABLED ? "ON" : "OFF", 
                  ALL_LOGS_ENABLED ? "ON" : "OFF");
  }

  // Load targets from config.env
  Serial.println("[MAIN] Loading targets...");
  loadTargetsFromConfig();
  Serial.printf("[MAIN] %d targets loaded successfully!\n", N_TARGETS);
  
  // Connect to WiFi first
  LOGLN("[MAIN] Conectando ao WiFi...");
  if (!Net::connectWiFi(WIFI_SSID, WIFI_PASS)) {
    LOGLN("[MAIN] ERROR: Failed to connect to WiFi!");
    // Continue anyway, maybe WiFi will connect later
  } else {
    LOGLN("[MAIN] WiFi connected successfully!");
    Net::printInfo();
  }
  
  // Initialize NTP after WiFi
  Serial.println("[MAIN] Initializing NTP...");
  if (!NTPManager::begin()) {
    Serial.println("[MAIN] ERROR: Failed to initialize NTP!");
    Serial.println("[MAIN] Continuing without NTP...");
  } else {
    Serial.println("[MAIN] NTP initialized successfully!");
  }
  
  // ===== CONFIGURATION MANAGEMENT =====
  // Initialize SDConfigManager after WiFi and NTP are ready
  Serial.println("[MAIN] Initializing SDConfigManager...");
  if (!SDConfigManager::begin()) {
    Serial.println("[MAIN] ERROR: Failed to initialize SDConfigManager!");
    Serial.println("[MAIN] Continuing without SD...");
  } else {
    Serial.println("[MAIN] SDConfigManager initialized!");
    
    // Check if SD has newer config than SPIFFS
    if (SDConfigManager::hasNewerConfig()) {
      Serial.println("[MAIN] SD has newer config! Copying to SPIFFS...");
      
      if (SDConfigManager::copySDToSPIFFS()) {
        Serial.println("[MAIN] Config copied successfully! Restarting...");
        delay(3000); // Give time for logs and avoid loops
        ESP.restart(); // Restart to load new config
      } else {
        Serial.println("[MAIN] ERROR: Failed to copy config from SD!");
        Serial.println("[MAIN] Continuing with SPIFFS config...");
      }
    } else {
      Serial.println("[MAIN] SPIFFS already has the latest config!");
    }
  }
  
  start_time = millis(); // Initialize start time for uptime calculation

  // Initialize display
  Serial.println("[MAIN] Initializing display...");
  if (!DisplayManager::begin()) {
    Serial.println("[MAIN] ERROR: Failed to initialize display!");
    return;
  }
  Serial.println("[MAIN] Display initialized successfully!");

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
    LOGLN("[MAIN] ERROR: Failed to initialize touch!");
    return;
  }
  LOGLN("[MAIN] Touch initialized successfully!");

  // Initialize network scanner
  if (!ScanManager::begin(targets, N_TARGETS)) {
    LOGLN("[MAIN] ERROR: Failed to initialize scanner!");
    return;
  }
  // Wire scan callbacks to post events for the Display task (no direct LVGL calls here)
  ScanManager::setCallbacks(
    [](){
      if (scan_event_queue) {
        ScanEvent ev{EV_SCAN_START, -1, UNKNOWN, 0};
        xQueueSend(scan_event_queue, &ev, 0);
      }
    },
    [](){
      if (scan_event_queue) {
        ScanEvent ev{EV_SCAN_COMPLETE, -1, UNKNOWN, 0};
        xQueueSend(scan_event_queue, &ev, 0);
      }
    },
    [](int idx, Status status, uint16_t latency){
      if (scan_event_queue) {
        ScanEvent ev{EV_TARGET_UPDATE, idx, status, latency};
        xQueueSend(scan_event_queue, &ev, 0);
      }
    }
  );
  scanner_initialized = true;
  LOGLN("[MAIN] Scanner initialized successfully!");

  // Initialize Telegram alerts
  if (initTelegramAlerts()) {
    LOGLN("[MAIN] Telegram alerts system initialized!");
    telegram_initialized = true;
    // Send initialization message
    delay(2000); // Wait a bit
    sendTestTelegramAlert(targets, N_TARGETS);
  } else {
    LOGLN("[MAIN] Telegram alerts system not initialized (configuration required)");
    telegram_initialized = false;
  }
  
  // Check Telegram status
  LOGF("[MAIN] Telegram status: %s\n", telegram_initialized ? "INITIALIZED" : "NOT INITIALIZED");

  // Initialize LVGL screen
  main_screen = lv_scr_act();
  lv_obj_set_style_bg_color(main_screen, lv_color_hex(0x000000), LV_PART_MAIN); // Black background

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

  // Clean interface design - no buttons, just status list
  Serial.println("[MAIN] Clean interface - no buttons, just status list!");

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

  // Create event queue and tasks
  scan_event_queue = xQueueCreate(20, sizeof(ScanEvent));

  // Start scanning after everything is initialized
  ScanManager::startScanning();
  Serial.println("[MAIN] Scanner started!");

  // Create tasks pinned to specific cores
  // Display/UI task on Core 1 with higher priority
  xTaskCreatePinnedToCore(
    displayTask,
    "DisplayTask",
    6144,
    nullptr,
    3,
    &display_task_handle,
    1
  );

  // Scanner task on Core 0 with slightly lower priority
  xTaskCreatePinnedToCore(
    scannerTask,
    "ScannerTask",
    6144,
    nullptr,
    2,
    &scanner_task_handle,
    0
  );

  Serial.println("[MAIN] Setup complete! Interface ready!");
}

void loop() {
  // All work is done in FreeRTOS tasks now
  delay(1000);
}
