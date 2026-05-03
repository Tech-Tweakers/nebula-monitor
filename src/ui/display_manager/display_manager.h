#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "core/domain/status/status.h"
#include "core/domain/target/target.h"

extern TFT_eSPI* tft;

class DisplayManager {
private:
  bool initialized;
  int footer_mode;
  unsigned long last_uptime_update;
  static const unsigned long UPTIME_UPDATE_INTERVAL = 500;
  volatile bool pending_refresh;
  bool modal_open;
  int modal_target_index;

  Target* targets;
  int targetCount;

  // Layout constants
  static const int TITLE_H     = 40;
  static const int ITEM_H      = 34;
  static const int ITEM_GAP    = 4;
  static const int ITEM_X      = 8;
  static const int ITEM_W      = 224;
  static const int ITEMS_Y     = 48;
  static const int FOOTER_H    = 30;
  static const int FOOTER_Y    = 286;

  // Colors
  static const uint32_t C_BG        = 0x000000;
  static const uint32_t C_TITLE_BG  = 0x1a1a2e;
  static const uint32_t C_ITEM_BG   = 0x111111;
  static const uint32_t C_UP_FAST   = 0x00AA55;
  static const uint32_t C_UP_SLOW   = 0x0055AA;
  static const uint32_t C_DOWN      = 0xAA1111;
  static const uint32_t C_UNKNOWN   = 0x333333;
  static const uint32_t C_FOOTER_BG = 0x1a1a1a;
  static const uint32_t C_TEXT      = 0xCCCCCC;
  static const uint32_t C_WHITE     = 0xFFFFFF;
  static const uint32_t C_DIM       = 0x666666;
  static const uint32_t C_MODAL_BG  = 0x0d0d0d;
  static const uint32_t C_SEP       = 0x2a2a2a;

  void drawTitleBar();
  void drawStatusItem(int index);
  void drawFooter();
  void drawModal(int index);
  void clearModal();
  void drawString(const String& text, int x, int y, uint8_t font, uint32_t color);
  uint32_t statusColor(Status s, uint16_t latency);

  String getFooterText() const;

public:
  DisplayManager();
  ~DisplayManager() {}

  bool initialize();
  void setTargets(Target* t, int count);
  void update();
  void updateTargetStatus(int index, Status status, uint16_t latency);
  void onScanStarted();
  void onScanCompleted();
  bool isInitialized() const { return initialized; }

  // Touch
  void handleTouch();
  void onFooterTouched();
  void onStatusItemTouched(int index);
  void openDetailModal(int index);
  void closeDetailModal();

  // UI
  void drawMainScreen();
  void updateFooter();
  void cycleFooterMode();
  void updateStatusItem(int index);
};
