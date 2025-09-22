#pragma once
#include <lvgl.h>
#include "core/domain/status/status.h"
#include "core/domain/target/target.h"
#include "core/domain/config/constants.h"
#include <Arduino.h>

class DisplayManager {
private:
  // LVGL objects
  lv_obj_t* main_screen;
  lv_obj_t* title_label;
  lv_obj_t** status_labels;
  lv_obj_t** name_labels;
  lv_obj_t** latency_labels;
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
  int maxTargets;
  
  // Tab system
  int currentTab;
  int totalTabs;
  lv_obj_t* tab_container;
  lv_obj_t* tab_buttons[MAX_TABS];
  lv_obj_t* tab_content[MAX_TABS];
  
public:
  DisplayManager();
  ~DisplayManager();
  
  // Initialization
  bool initialize();
  void setTargets(Target* targets, int count, int maxCount);
  bool allocateUIArrays();
  void deallocateUIArrays();
  
  // Main operations
  void update();
  void updateTargetStatus(int index, Status status, uint16_t latency);
  
  // Event handlers
  void onScanStarted();
  void onScanCompleted();
  
  // UI Management
  void createMainScreen();
  void createTabSystem();
  void createStatusItems();
  void createFooter();
  void updateFooter();
  void cycleFooterMode();
  
  // Tab management
  void switchToTab(int tabIndex);
  void updateTabButtons();
  void createTabContent(int tabIndex);
  
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
