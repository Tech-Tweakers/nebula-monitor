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
  static const int TITLE_H     = 44;
  static const int ITEM_H      = 36;
  static const int ITEM_GAP    = 5;
  static const int ITEM_X      = 10;
  static const int ITEM_W      = 220;
  static const int ITEMS_Y     = 52;
  static const int FOOTER_H    = 28;
  static const int FOOTER_Y    = 288;
  static const int BAR_W       = 4;   // status bar width

  // Palette — RGB values passed to tft->color565(r,g,b) — TFT_eSPI handles BGR
  // Stored as 0xRRGGBB for use with the RGB macro below
  static const uint32_t C_BG        = 0x0d1117;
  static const uint32_t C_TITLE_BG  = 0x161b22;
  static const uint32_t C_ITEM_BG   = 0x161b22;
  static const uint32_t C_UP_FAST   = 0x00ff41;
  static const uint32_t C_UP_SLOW   = 0xffaa00;
  static const uint32_t C_DOWN      = 0xff4444;
  static const uint32_t C_UNKNOWN   = 0x444444;
  static const uint32_t C_FOOTER_BG = 0x161b22;
  static const uint32_t C_TEXT      = 0xc9d1d9;
  static const uint32_t C_WHITE     = 0xffffff;
  static const uint32_t C_DIM       = 0x8b949e;
  static const uint32_t C_MODAL_BG  = 0x0d1117;
  static const uint32_t C_SEP       = 0x21262d;
  static const uint32_t C_ACCENT    = 0x58a6ff;

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
