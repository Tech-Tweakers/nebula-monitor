#pragma once
#include <lvgl.h>
#include "core/domain/status.h"
#include "core/domain/target.h"
#include <Arduino.h>

class DisplayManager {
private:
  // LVGL objects
  lv_obj_t* main_screen;
  lv_obj_t* title_label;
  lv_obj_t* status_labels[6];
  lv_obj_t* name_labels[6];
  lv_obj_t* latency_labels[6];
  lv_obj_t* footer;
  lv_obj_t* footer_label;
  
  // State
  bool initialized;
  int footer_mode;
  unsigned long last_uptime_update;
  static const unsigned long UPTIME_UPDATE_INTERVAL = 500;
  
  // Target references
  Target* targets;
  int targetCount;
  
public:
  DisplayManager();
  ~DisplayManager();
  
  // Initialization
  bool initialize();
  void setTargets(Target* targets, int count);
  
  // Main operations
  void update();
  void updateTargetStatus(int index, Status status, uint16_t latency);
  
  // Event handlers
  void onScanStarted();
  void onScanCompleted();
  
  // UI Management
  void createMainScreen();
  void createStatusItems();
  void createFooter();
  void updateFooter();
  void cycleFooterMode();
  
  // Touch handling
  void handleTouch();
  void onFooterTouched();
  void onStatusItemTouched(int index);
  
  // Getters
  bool isInitialized() const { return initialized; }
  
private:
  // Internal UI methods
  void updateStatusItem(int index);
  void updateFooterContent();
  String getFooterText() const;
  void setStatusItemColor(int index, Status status, uint16_t latency);
};
