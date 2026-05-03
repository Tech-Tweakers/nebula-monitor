#include "ui/display_manager/display_manager.h"
#include "ui/touch_handler/touch_handler.h"
#include "ui/led_controller/led_controller.h"
#include <Arduino.h>
#include <WiFi.h>
#include "core/infrastructure/logger/logger.h"

// ── helpers ───────────────────────────────────────────────────────────────────

static inline uint16_t c(uint32_t rgb) {
  uint8_t r = (rgb >> 16) & 0xFF;
  uint8_t g = (rgb >> 8)  & 0xFF;
  uint8_t b =  rgb        & 0xFF;
  return ~tft->color565(r, g, b);
}

// ── lifecycle ─────────────────────────────────────────────────────────────────

DisplayManager::DisplayManager()
  : initialized(false), footer_mode(0), last_uptime_update(0),
    pending_refresh(false), modal_open(false), modal_target_index(-1),
    targets(nullptr), targetCount(0) {}

bool DisplayManager::initialize() {
  if (initialized) return true;
  Serial_println("[DISPLAY] Initializing...");

  TouchHandler::initialize();
  LEDController::initialize();

  tft->fillScreen(c(C_BG));
  drawMainScreen();

  initialized = true;
  Serial_println("[DISPLAY] Ready.");
  return true;
}

void DisplayManager::setTargets(Target* t, int count) {
  targets = t;
  targetCount = count;
  if (initialized) drawMainScreen();
}

void DisplayManager::update() {
  if (!initialized) return;

  if (pending_refresh) {
    pending_refresh = false;
    if (modal_open) drawModal(modal_target_index);
    else drawMainScreen();
  }

  handleTouch();

  if (millis() - last_uptime_update >= UPTIME_UPDATE_INTERVAL) {
    if (!modal_open) drawFooter();
    last_uptime_update = millis();
  }

  LEDController::update();
}

void DisplayManager::updateTargetStatus(int index, Status status, uint16_t latency) {
  if (index < 0 || index >= targetCount || !targets) return;
  Serial_printf("[DISPLAY] updateTargetStatus: index=%d status=%d latency=%d\n", index, status, latency);
  targets[index].setStatus(status);
  targets[index].setLatency(latency);
  pending_refresh = true;
}

void DisplayManager::onScanStarted() {
  LEDController::setStatus(LEDStatus::SCANNING);
}

void DisplayManager::onScanCompleted() {
  bool anyDown = false;
  for (int i = 0; i < targetCount; i++) {
    if (targets[i].isDown()) { anyDown = true; break; }
  }
  LEDController::setStatus(anyDown ? LEDStatus::TARGETS_DOWN : LEDStatus::SYSTEM_OK);
  pending_refresh = true;
}

// ── drawing ───────────────────────────────────────────────────────────────────

void DisplayManager::drawMainScreen() {
  tft->fillScreen(c(C_BG));
  drawTitleBar();
  for (int i = 0; i < targetCount && i < 6; i++) drawStatusItem(i);
  drawFooter();
}

void DisplayManager::drawTitleBar() {
  tft->fillRect(0, 0, 240, TITLE_H, c(C_TITLE_BG));
  // accent line at bottom of title
  tft->drawFastHLine(0, TITLE_H - 1, 240, c(C_ACCENT));

  // Draw title right-aligned
  tft->setTextSize(1);
  int nebula_w = tft->textWidth("NEBULA ", 2);
  int monitor_w = tft->textWidth("MONITOR", 2);
  int start_x = 12;
  tft->setTextColor(c(C_WHITE), c(C_TITLE_BG));
  tft->setTextDatum(ML_DATUM);
  tft->drawString("NEBULA ", start_x, TITLE_H / 2, 2);
  tft->setTextColor(c(C_ACCENT), c(C_TITLE_BG));
  tft->drawString("MONITOR", start_x + nebula_w, TITLE_H / 2, 2);

  tft->setTextColor(c(C_DIM), c(C_TITLE_BG));
  tft->setTextDatum(MR_DATUM);
  tft->drawString("v2.4.1", 232, TITLE_H / 2, 1);
}

void DisplayManager::drawStatusItem(int index) {
  if (!targets || index >= targetCount) return;

  Target& t = targets[index];
  int y = ITEMS_Y + index * (ITEM_H + ITEM_GAP);
  uint32_t sc = statusColor(t.getStatus(), t.getLatency());

  // Item background
  tft->fillRoundRect(ITEM_X, y, ITEM_W, ITEM_H, 3, c(C_ITEM_BG));

  // Left status bar
  tft->fillRoundRect(ITEM_X, y, BAR_W, ITEM_H, 2, c(sc));

  // Target name
  tft->setTextColor(c(C_TEXT), c(C_ITEM_BG));
  tft->setTextDatum(ML_DATUM);
  tft->setTextSize(1);
  tft->drawString(t.getName(), ITEM_X + BAR_W + 8, y + ITEM_H / 2, 2);

  // Latency — colored by status
  uint32_t lat_col = t.isDown() ? C_DOWN : (t.getLatency() < 500 ? C_UP_FAST : C_UP_SLOW);
  tft->setTextColor(c(lat_col), c(C_ITEM_BG));
  tft->setTextDatum(MR_DATUM);
  tft->drawString(t.getLatencyText(), ITEM_X + ITEM_W - 8, y + ITEM_H / 2, 2);
}

void DisplayManager::drawFooter() {
  tft->fillRect(0, FOOTER_Y, 240, FOOTER_H, c(C_FOOTER_BG));
  tft->drawFastHLine(0, FOOTER_Y, 240, c(C_SEP));

  String text = getFooterText();
  tft->setTextColor(c(C_DIM), c(C_FOOTER_BG));
  tft->setTextDatum(MC_DATUM);
  tft->drawString(text, 120, FOOTER_Y + FOOTER_H / 2, 2);
}

void DisplayManager::drawModal(int index) {
  if (!targets || index < 0 || index >= targetCount) return;
  Target& t = targets[index];
  uint32_t sc = statusColor(t.getStatus(), t.getLatency());

  tft->fillScreen(c(C_MODAL_BG));

  // Header with status color accent
  tft->fillRect(0, 0, 240, TITLE_H, c(C_TITLE_BG));
  tft->drawFastHLine(0, TITLE_H - 1, 240, c(sc));

  // Status dot + name
  tft->fillCircle(18, TITLE_H / 2, 5, c(sc));
  tft->setTextColor(c(C_WHITE), c(C_TITLE_BG));
  tft->setTextDatum(ML_DATUM);
  tft->drawString(t.getName(), 30, TITLE_H / 2, 2);

  // X button
  tft->setTextColor(c(C_DIM), c(C_TITLE_BG));
  tft->setTextDatum(MR_DATUM);
  tft->drawString("[X]", 234, TITLE_H / 2, 2);

  // Separator
  tft->drawFastHLine(0, TITLE_H + 8, 240, c(C_SEP));

  // Content rows
  int row_y = TITLE_H + 24;
  int row_gap = 28;

  auto drawRow = [&](const String& label, const String& value, uint32_t val_color) {
    tft->setTextColor(c(C_DIM), c(C_MODAL_BG));
    tft->setTextDatum(ML_DATUM);
    tft->drawString(label, 12, row_y, 1);
    tft->setTextColor(c(val_color), c(C_MODAL_BG));
    tft->setTextDatum(MR_DATUM);
    tft->drawString(value, 228, row_y, 1);
    tft->drawFastHLine(12, row_y + 10, 216, c(C_SEP));
    row_y += row_gap;
  };

  drawRow("STATUS", t.getStatusText(), sc);
  drawRow("LATENCY", t.getLatencyText(), t.isDown() ? C_DOWN : C_TEXT);
  drawRow("FAILURES", String(t.getFailCount()), t.getFailCount() > 0 ? C_UP_SLOW : C_DIM);

  String down_text;
  if (t.getLastDownDuration() > 0) {
    unsigned long secs = t.getLastDownDuration() / 1000;
    down_text = secs < 60 ? String(secs) + "s" : String(secs / 60) + "m" + String(secs % 60) + "s";
  } else { down_text = "--"; }
  drawRow("LAST DOWN", down_text, C_DIM);

  String since_text;
  if (t.getLastStatusChange() > 0) {
    unsigned long secs = (millis() - t.getLastStatusChange()) / 1000;
    if (secs < 60) since_text = String(secs) + "s";
    else if (secs < 3600) since_text = String(secs / 60) + "m";
    else since_text = String(secs / 3600) + "h" + String((secs % 3600) / 60) + "m";
  } else { since_text = "--"; }
  drawRow("SINCE", since_text, C_DIM);
}

// ── touch ─────────────────────────────────────────────────────────────────────

void DisplayManager::handleTouch() {
  if (!TouchHandler::isTouched()) return;

  int16_t x, y;
  TouchHandler::getTouchCoordinates(x, y);

  int16_t ty = y + 15; // Y offset correction

  if (modal_open) {
    if (x >= 190 && x <= 240 && ty >= 6 && ty <= 44) closeDetailModal();
    return;
  }

  if (ty >= FOOTER_Y && ty <= FOOTER_Y + FOOTER_H) {
    onFooterTouched();
    return;
  }

  for (int i = 0; i < targetCount && i < 6; i++) {
    int iy = ITEMS_Y + i * (ITEM_H + ITEM_GAP);
    if (x >= ITEM_X && x <= ITEM_X + ITEM_W && ty >= iy && ty < iy + ITEM_H) {
      onStatusItemTouched(i);
      return;
    }
  }
}

void DisplayManager::onFooterTouched()          { cycleFooterMode(); }
void DisplayManager::cycleFooterMode()          { footer_mode = (footer_mode + 1) % 3; drawFooter(); }
void DisplayManager::onStatusItemTouched(int i) { if (i >= 0 && i < targetCount) openDetailModal(i); }
void DisplayManager::updateFooter()             { if (!modal_open) drawFooter(); }
void DisplayManager::updateStatusItem(int i)    { if (!modal_open) drawStatusItem(i); }

void DisplayManager::openDetailModal(int index) {
  modal_open = true;
  modal_target_index = index;
  drawModal(index);
}

void DisplayManager::closeDetailModal() {
  modal_open = false;
  modal_target_index = -1;
  drawMainScreen();
}

// ── helpers ───────────────────────────────────────────────────────────────────

uint32_t DisplayManager::statusColor(Status s, uint16_t latency) {
  if (s == UP)   return latency < 500 ? C_UP_FAST : C_UP_SLOW;
  if (s == DOWN) return C_DOWN;
  return C_UNKNOWN;
}

String DisplayManager::getFooterText() const {
  if (!targets) return "no targets";

  switch (footer_mode) {
    case 0: {
      int alerts = 0, up = 0;
      for (int i = 0; i < targetCount; i++) {
        if (targets[i].isDown()) alerts++;
        if (targets[i].isHealthy()) up++;
      }
      unsigned long secs = millis() / 1000;
      String uptime = String(secs / 3600) + "h" + String((secs % 3600) / 60) + "m";
      return "alerts:" + String(alerts) + "  up:" + String(up) + "/" + String(targetCount) + "  " + uptime;
    }
    case 1: {
      return "ip:" + WiFi.localIP().toString() + "  " + String(WiFi.RSSI()) + "dBm";
    }
    case 2: {
      uint32_t free = ESP.getFreeHeap();
      uint32_t total = ESP.getHeapSize();
      uint32_t pct = (total - free) * 100 / total;
      return "ram:" + String(pct) + "%  free:" + String(free / 1024) + "KB";
    }
    default: return "";
  }
}
