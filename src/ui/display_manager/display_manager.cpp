#include "ui/display_manager/display_manager.h"
#include "ui/touch_handler/touch_handler.h"
#include "ui/led_controller/led_controller.h"
#include <Arduino.h>
#include <WiFi.h>
#include "core/infrastructure/logger/logger.h"

DisplayManager::DisplayManager() 
  : main_screen(nullptr), title_label(nullptr), footer(nullptr), footer_label(nullptr),
    initialized(false), footer_mode(0), last_uptime_update(0), targets(nullptr), targetCount(0) {
  
  // Initialize status arrays
  for (int i = 0; i < 6; i++) {
    status_labels[i] = nullptr;
    name_labels[i] = nullptr;
    latency_labels[i] = nullptr;
  }
}

DisplayManager::~DisplayManager() {
  // LVGL objects are cleaned up automatically
}

bool DisplayManager::initialize() {
  if (initialized) return true;
  
  Serial_println("[DISPLAY] Initializing display manager...");
  
  // Initialize LVGL (this should be done in main setup)
  // lv_init() is called in main.cpp
  
  // Create main screen
  createMainScreen();
  
  // Initialize touch handler
  TouchHandler::initialize();
  
  // Initialize LED controller
  LEDController::initialize();
  
  initialized = true;
  Serial_println("[DISPLAY] Display manager initialized successfully!");
  
  return true;
}

void DisplayManager::setTargets(Target* targets, int count) {
  this->targets = targets;
  this->targetCount = count;
  
  // Update status items if already created
  if (initialized) {
    createStatusItems();
  }
}

void DisplayManager::update() {
  if (!initialized) return;
  
  // Handle LVGL tasks
  lv_timer_handler();
  
  // Handle touch input
  handleTouch();
  
  // Update footer periodically
  if (millis() - last_uptime_update >= UPTIME_UPDATE_INTERVAL) {
    updateFooter();
    last_uptime_update = millis();
  }
  
  // Update LED status
  LEDController::update();
}

void DisplayManager::updateTargetStatus(int index, Status status, uint16_t latency) {
  if (index < 0 || index >= targetCount || !targets) return;
  
  Serial_printf("[DISPLAY] updateTargetStatus called: index=%d, status=%d, latency=%d\n", 
               index, status, latency);
  
  targets[index].setStatus(status);
  targets[index].setLatency(latency);
  
  updateStatusItem(index);
}

void DisplayManager::onScanStarted() {
  Serial_println("[DISPLAY] Scan started");
  LEDController::setStatus(LEDStatus::SCANNING);
  updateFooter();
}

void DisplayManager::onScanCompleted() {
  Serial_println("[DISPLAY] Scan completed");
  
  // Determine LED status based on targets
  bool anyDown = false;
  for (int i = 0; i < targetCount; i++) {
    if (targets[i].isDown()) {
      anyDown = true;
      break;
    }
  }
  
  LEDController::setStatus(anyDown ? LEDStatus::TARGETS_DOWN : LEDStatus::SYSTEM_OK);
  updateFooter();
}

void DisplayManager::createMainScreen() {
  // Get main screen
  main_screen = lv_scr_act();
  lv_obj_set_style_bg_color(main_screen, lv_color_hex(0x000000), LV_PART_MAIN);
  
  // Create title bar
  lv_obj_t* title_bar = lv_obj_create(main_screen);
  lv_obj_set_size(title_bar, 240, 40);
  lv_obj_set_pos(title_bar, 0, 0);
  lv_obj_set_style_bg_color(title_bar, lv_color_hex(0x2d2d2d), LV_PART_MAIN);
  lv_obj_set_style_border_width(title_bar, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(title_bar, 0, LV_PART_MAIN);
  
  // Create title label
  title_label = lv_label_create(title_bar);
  lv_label_set_text(title_label, "Nebula Monitor v2.5");
  lv_obj_set_style_text_color(title_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_center(title_label);
  
  // Create main form container
  lv_obj_t* main_form = lv_obj_create(main_screen);
  lv_obj_set_size(main_form, 220, 220);
  lv_obj_set_pos(main_form, 10, 50);
  lv_obj_set_style_bg_color(main_form, lv_color_hex(0x222222), LV_PART_MAIN);
  lv_obj_set_style_border_width(main_form, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(main_form, 8, LV_PART_MAIN);
  lv_obj_set_style_pad_all(main_form, 10, LV_PART_MAIN);
  
  // Enable flex layout
  lv_obj_set_flex_flow(main_form, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(main_form, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  
  // Create status items
  createStatusItems();
  
  // Create footer
  createFooter();
}

void DisplayManager::createStatusItems() {
  if (!targets) {
    Serial_println("[DISPLAY] ERROR: No targets provided!");
    return;
  }
  
  Serial_printf("[DISPLAY] Creating status items for %d targets\n", targetCount);
  
  // Find the main form container (it should be the second child of main_screen)
  lv_obj_t* main_form = nullptr;
  if (lv_obj_get_child_cnt(main_screen) >= 2) {
    main_form = lv_obj_get_child(main_screen, 1); // main_form is the second child (index 1)
  }
  
  if (!main_form) {
    Serial_println("[DISPLAY] ERROR: Main form not found!");
    return;
  }
  
  // Create status items for each target
  for (int i = 0; i < targetCount && i < 6; i++) {
    Serial_printf("[DISPLAY] Creating status item %d for target: %s\n", i, targets[i].getName().c_str());
    
    // Status item container
    lv_obj_t* status_item = lv_obj_create(main_form);
    lv_obj_set_size(status_item, 200, 24);
    lv_obj_set_style_bg_color(status_item, lv_color_hex(0x111111), LV_PART_MAIN);
    lv_obj_set_style_border_width(status_item, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(status_item, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_all(status_item, 4, LV_PART_MAIN);
    
    // Enable flex layout for the item
    lv_obj_set_flex_flow(status_item, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_item, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Target name label
    lv_obj_t* name_label = lv_label_create(status_item);
    lv_label_set_text(name_label, targets[i].getName().c_str());
    lv_obj_set_style_text_color(name_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    
    // Latency label
    lv_obj_t* latency_label = lv_label_create(status_item);
    lv_label_set_text(latency_label, "--- ms");
    lv_obj_set_style_text_color(latency_label, lv_color_hex(0xCCCCCC), LV_PART_MAIN);
    
    // Store references
    status_labels[i] = status_item;
    name_labels[i] = name_label;
    latency_labels[i] = latency_label;
    
    Serial_printf("[DISPLAY] Status item %d created successfully\n", i);
    
    // Update initial status
    updateStatusItem(i);
  }
}

void DisplayManager::createFooter() {
  footer = lv_obj_create(main_screen);
  lv_obj_set_size(footer, 220, 32);
  lv_obj_set_pos(footer, 10, 260);
  lv_obj_set_style_bg_color(footer, lv_color_hex(0x333333), LV_PART_MAIN);
  lv_obj_set_style_border_width(footer, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(footer, 6, LV_PART_MAIN);
  lv_obj_set_style_pad_all(footer, 6, LV_PART_MAIN);
  
  // Enable flex layout
  lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  
  // Footer label
  footer_label = lv_label_create(footer);
  lv_label_set_text(footer_label, "Warming up...");
  lv_obj_set_style_text_color(footer_label, lv_color_hex(0xCCCCCC), LV_PART_MAIN);
  lv_obj_set_width(footer_label, LV_PCT(100));
  lv_obj_set_style_text_align(footer_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  
  // Make footer clickable
  lv_obj_add_event_cb(footer, [](lv_event_t* e) {
    DisplayManager* dm = static_cast<DisplayManager*>(lv_event_get_user_data(e));
    if (dm) dm->onFooterTouched();
  }, LV_EVENT_CLICKED, this);
}

void DisplayManager::updateFooter() {
  if (!footer_label) return;
  
  String text = getFooterText();
  lv_label_set_text(footer_label, text.c_str());
  
  // Force a gentle refresh to show changes
  if (footer && lv_obj_is_valid(footer)) {
    lv_obj_invalidate(footer);
    // Also invalidate parent to ensure full refresh
    lv_obj_invalidate(lv_obj_get_parent(footer));
    // Force immediate refresh of just the footer area
    lv_obj_refresh_ext_draw_size(footer);
  }
}

void DisplayManager::cycleFooterMode() {
  footer_mode = (footer_mode + 1) % 3;
  updateFooter();
}

void DisplayManager::handleTouch() {
  if (TouchHandler::isTouched()) {
    int16_t x, y;
    TouchHandler::getTouchCoordinates(x, y);
    
    // Check footer touch
    lv_area_t footer_area;
    lv_obj_get_coords(footer, &footer_area);
    if (x >= footer_area.x1 && x < footer_area.x2 && y >= footer_area.y1 && y < footer_area.y2) {
      onFooterTouched();
      return;
    }
    
    // Check status item touches
    for (int i = 0; i < targetCount && i < 6; i++) {
      if (status_labels[i]) {
        lv_area_t item_area;
        lv_obj_get_coords(status_labels[i], &item_area);
        if (x >= item_area.x1 && x < item_area.x2 && y >= item_area.y1 && y < item_area.y2) {
          onStatusItemTouched(i);
          break;
        }
      }
    }
  }
}

void DisplayManager::onFooterTouched() {
  cycleFooterMode();
}

void DisplayManager::onStatusItemTouched(int index) {
  if (index < 0 || index >= targetCount) return;
  
  // Could show detail window here
}

void DisplayManager::updateStatusItem(int index) {
  if (index < 0 || index >= targetCount || !targets) return;
  
  // Additional safety checks
  if (index >= 6) return;
  
  Target& target = targets[index];
  
  Serial_printf("[DISPLAY] Updating status item %d: %s - %s (%d ms)\n", 
               index, target.getName().c_str(), 
               target.getStatusText().c_str(), target.getLatency());
  
  // Update latency label with safety check
  if (latency_labels[index] && lv_obj_is_valid(latency_labels[index])) {
    String latencyText = target.getLatencyText();
    lv_label_set_text(latency_labels[index], latencyText.c_str());
    Serial_printf("[DISPLAY] Updated latency label %d: %s\n", index, latencyText.c_str());
  } else {
    Serial_printf("[DISPLAY] ERROR: Latency label %d is null or invalid!\n", index);
  }
  
  // Update colors with safety check
  if (status_labels[index] && lv_obj_is_valid(status_labels[index])) {
    setStatusItemColor(index, target.getStatus(), target.getLatency());
  } else {
    Serial_printf("[DISPLAY] ERROR: Status label %d is null or invalid!\n", index);
  }
  
  // Force a gentle refresh to show changes
  if (status_labels[index] && lv_obj_is_valid(status_labels[index])) {
    lv_obj_invalidate(status_labels[index]);
    // Also invalidate parent to ensure full refresh
    lv_obj_invalidate(lv_obj_get_parent(status_labels[index]));
  }
  
  // Force immediate refresh for critical updates
  lv_refr_now(NULL);
}

void DisplayManager::setStatusItemColor(int index, Status status, uint16_t latency) {
  if (index < 0 || index >= 6 || !status_labels[index]) return;
  
  // Additional safety check
  if (!lv_obj_is_valid(status_labels[index])) return;
  
  lv_color_t bg_color, text_color;
  
  if (status == UP) {
    if (latency < 500) {
      bg_color = lv_color_hex(0xFF00FF); // Green for good latency
      text_color = lv_color_hex(0xFFFFFF);
    } else {
      bg_color = lv_color_hex(0x0086ff); // Blue for slow latency
      text_color = lv_color_hex(0x000000);
    }
  } else if (status == DOWN) {
    bg_color = lv_color_hex(0x00FFFF); // Red for down
    text_color = lv_color_hex(0x000000);
  } else {
    bg_color = lv_color_hex(0x111111); // Gray for unknown
    text_color = lv_color_hex(0xCCCCCC);
  }
  
  lv_obj_set_style_bg_color(status_labels[index], bg_color, LV_PART_MAIN);
  
  if (name_labels[index] && lv_obj_is_valid(name_labels[index])) {
    lv_obj_set_style_text_color(name_labels[index], text_color, LV_PART_MAIN);
  }
  if (latency_labels[index] && lv_obj_is_valid(latency_labels[index])) {
    lv_obj_set_style_text_color(latency_labels[index], text_color, LV_PART_MAIN);
  }
  
  lv_obj_invalidate(status_labels[index]);
}

String DisplayManager::getFooterText() const {
  if (!targets) return "No targets";
  
  switch (footer_mode) {
    case 0: { // System Overview
      int active_alerts = 0, targets_up = 0;
      for (int i = 0; i < targetCount; i++) {
        if (targets[i].isDown()) active_alerts++;
        if (targets[i].isHealthy()) targets_up++;
      }
      
      // Calculate uptime
      unsigned long uptime_ms = millis();
      unsigned long hours = uptime_ms / 3600000;
      unsigned long minutes = (uptime_ms % 3600000) / 60000;
      String uptime_str = String(hours) + ":" + (minutes < 10 ? "0" : "") + String(minutes);
      
      return "Alerts: " + String(active_alerts) + " | On: " + String(targets_up) + "/" + String(targetCount) + " | Up: " + uptime_str;
    }
    case 1: { // Network Info
      // Get dynamic WiFi info
      String ip = WiFi.localIP().toString();
      int32_t rssi = WiFi.RSSI();
      String rssi_str = String(rssi) + " dBm";
      
      return "IP: " + ip + " | " + rssi_str;
    }
    case 2: { // Performance
      // Get dynamic memory info
      uint32_t free_heap = ESP.getFreeHeap();
      uint32_t total_heap = ESP.getHeapSize();
      uint32_t heap_percent = (total_heap - free_heap) * 100 / total_heap;
      
      // Get free PSRAM if available
      uint32_t free_psram = ESP.getFreePsram();
      String rom_str = String(free_psram / 1024) + "KB";
      if (free_psram == 0) {
        rom_str = String(free_heap / 1024) + "KB";
      }
      
      return "Cpu: 45% | Ram: " + String(heap_percent) + "% | HP: " + rom_str;
    }
    default:
      return "Unknown mode";
  }
}
