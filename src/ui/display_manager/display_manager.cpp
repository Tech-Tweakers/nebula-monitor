#include "ui/display_manager/display_manager.h"
#include "ui/touch_handler/touch_handler.h"
#include "ui/led_controller/led_controller.h"
#include <Arduino.h>
#include <WiFi.h>
#include "core/infrastructure/logger/logger.h"

DisplayManager::DisplayManager()
  : initialized(false), footer_mode(0), last_uptime_update(0),
    pending_refresh(false), modal_open(false), modal_target_index(-1),
    targets(nullptr), targetCount(0) {}

bool DisplayManager::initialize() {
  if (initialized) return true;
  Serial_println("[DISPLAY] Initializing...");

  TouchHandler::initialize();
  LEDController::initialize();

  tft->fillScreen(tft->color565(0, 0, 0));
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

// ── Drawing ──────────────────────────────────────────────────────────────────

void DisplayManager::drawMainScreen() {
  tft->fillScreen(tft->color565(0, 0, 0));
  drawTitleBar();
  for (int i = 0; i < targetCount && i < 6; i++) drawStatusItem(i);
  drawFooter();
}

void DisplayManager::drawTitleBar() {
  uint16_t bg = tft->color565(0x1a, 0x1a, 0x2e);
  tft->fillRect(0, 0, 240, TITLE_H, bg);
  tft->setTextColor(TFT_WHITE, bg);
  tft->setTextDatum(MC_DATUM);
  tft->setTextSize(1);
  tft->drawString("Nebula Monitor v2.4.1", 120, 20, 2);
}

void DisplayManager::drawStatusItem(int index) {
  if (!targets || index >= targetCount) return;

  Target& t = targets[index];
  int y = ITEMS_Y + index * (ITEM_H + ITEM_GAP);

  uint32_t bg32 = statusColor(t.getStatus(), t.getLatency());
  uint16_t bg = tft->color565((bg32 >> 16) & 0xFF, (bg32 >> 8) & 0xFF, bg32 & 0xFF);
  uint16_t fg = TFT_WHITE;

  tft->fillRoundRect(ITEM_X, y, ITEM_W, ITEM_H, 4, bg);

  tft->setTextColor(fg, bg);
  tft->setTextDatum(ML_DATUM);
  tft->setTextSize(1);
  tft->drawString(t.getName(), ITEM_X + 8, y + ITEM_H / 2, 2);

  tft->setTextDatum(MR_DATUM);
  tft->drawString(t.getLatencyText(), ITEM_X + ITEM_W - 8, y + ITEM_H / 2, 2);
}

void DisplayManager::drawFooter() {
  uint16_t bg = tft->color565(0x1a, 0x1a, 0x1a);
  tft->fillRect(0, FOOTER_Y, 240, FOOTER_H, bg);
  String text = getFooterText();
  tft->setTextColor(tft->color565(0xAA, 0xAA, 0xAA), bg);
  tft->setTextDatum(MC_DATUM);
  tft->setTextSize(1);
  tft->drawString(text, 120, FOOTER_Y + FOOTER_H / 2, 1);
}

void DisplayManager::drawModal(int index) {
  if (!targets || index < 0 || index >= targetCount) return;
  Target& t = targets[index];

  tft->fillScreen(tft->color565(0x0d, 0x0d, 0x0d));

  // Header
  uint16_t hdr_bg = tft->color565(0x1a, 0x1a, 0x2e);
  tft->fillRect(0, 0, 240, TITLE_H, hdr_bg);
  tft->setTextColor(TFT_WHITE, hdr_bg);
  tft->setTextDatum(ML_DATUM);
  tft->drawString(t.getName(), 12, 20, 2);

  // X button
  tft->fillCircle(216, 20, 14, tft->color565(0x33, 0x33, 0x33));
  tft->setTextColor(tft->color565(0xAA, 0xAA, 0xAA), tft->color565(0x33, 0x33, 0x33));
  tft->setTextDatum(MC_DATUM);
  tft->drawString("X", 216, 20, 2);

  // Separator
  tft->drawFastHLine(12, 46, 216, tft->color565(0x2a, 0x2a, 0x2a));

  // Status
  uint32_t sc = statusColor(t.getStatus(), t.getLatency());
  uint16_t status_col = tft->color565((sc >> 16) & 0xFF, (sc >> 8) & 0xFF, sc & 0xFF);
  tft->setTextDatum(ML_DATUM);
  tft->setTextColor(status_col, tft->color565(0x0d, 0x0d, 0x0d));
  tft->drawString("Status:   " + String(t.getStatusText()), 12, 64, 2);

  uint16_t dim = tft->color565(0xCC, 0xCC, 0xCC);
  uint16_t muted = tft->color565(0x88, 0x88, 0x88);
  uint16_t modal_bg = tft->color565(0x0d, 0x0d, 0x0d);

  tft->setTextColor(dim, modal_bg);
  tft->drawString("Latency:  " + t.getLatencyText(), 12, 92, 2);

  uint16_t fail_col = t.getFailCount() > 0 ? tft->color565(0xFF, 0x88, 0x00) : muted;
  tft->setTextColor(fail_col, modal_bg);
  tft->drawString("Failures: " + String(t.getFailCount()), 12, 120, 2);

  tft->setTextColor(muted, modal_bg);

  String down_text;
  if (t.getLastDownDuration() > 0) {
    unsigned long secs = t.getLastDownDuration() / 1000;
    if (secs < 60) down_text = "Last down: " + String(secs) + "s";
    else down_text = "Last down: " + String(secs / 60) + "m" + String(secs % 60) + "s";
  } else {
    down_text = "Last down: --";
  }
  tft->drawString(down_text, 12, 148, 2);

  String since_text;
  if (t.getLastStatusChange() > 0) {
    unsigned long secs = (millis() - t.getLastStatusChange()) / 1000;
    if (secs < 60) since_text = "Since:     " + String(secs) + "s";
    else if (secs < 3600) since_text = "Since:     " + String(secs / 60) + "m";
    else since_text = "Since:     " + String(secs / 3600) + "h" + String((secs % 3600) / 60) + "m";
  } else {
    since_text = "Since:     --";
  }
  tft->drawString(since_text, 12, 172, 2);
}

// ── Touch ─────────────────────────────────────────────────────────────────────

void DisplayManager::handleTouch() {
  if (!TouchHandler::isTouched()) return;

  int16_t x, y;
  TouchHandler::getTouchCoordinates(x, y);

  if (modal_open) {
    // X button area
    if (x >= 196 && x <= 236 && y >= 6 && y <= 40) closeDetailModal();
    return;
  }

  // Footer
  if (y >= FOOTER_Y && y <= FOOTER_Y + FOOTER_H) {
    onFooterTouched();
    return;
  }

  // Status items
  for (int i = 0; i < targetCount && i < 6; i++) {
    int iy = ITEMS_Y + i * (ITEM_H + ITEM_GAP);
    if (x >= ITEM_X && x <= ITEM_X + ITEM_W && (y + 15) >= iy && (y + 15) < iy + ITEM_H) {
      onStatusItemTouched(i);
      return;
    }
  }
}

void DisplayManager::onFooterTouched() {
  cycleFooterMode();
}

void DisplayManager::cycleFooterMode() {
  footer_mode = (footer_mode + 1) % 3;
  drawFooter();
}

void DisplayManager::onStatusItemTouched(int index) {
  if (index < 0 || index >= targetCount) return;
  openDetailModal(index);
}

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

void DisplayManager::updateFooter() {
  if (!modal_open) drawFooter();
}

void DisplayManager::updateStatusItem(int index) {
  if (!modal_open) drawStatusItem(index);
}

// ── Helpers ───────────────────────────────────────────────────────────────────

uint32_t DisplayManager::statusColor(Status s, uint16_t latency) {
  if (s == UP)   return latency < 500 ? C_UP_FAST : C_UP_SLOW;
  if (s == DOWN) return C_DOWN;
  return C_UNKNOWN;
}

String DisplayManager::getFooterText() const {
  if (!targets) return "No targets";

  switch (footer_mode) {
    case 0: {
      int alerts = 0, up = 0;
      for (int i = 0; i < targetCount; i++) {
        if (targets[i].isDown()) alerts++;
        if (targets[i].isHealthy()) up++;
      }
      unsigned long secs = millis() / 1000;
      String uptime = String(secs / 3600) + ":" + (((secs % 3600) / 60) < 10 ? "0" : "") + String((secs % 3600) / 60);
      return "Alerts:" + String(alerts) + " On:" + String(up) + "/" + String(targetCount) + " Up:" + uptime;
    }
    case 1: {
      String ip = WiFi.localIP().toString();
      return "IP:" + ip + " " + String(WiFi.RSSI()) + "dBm";
    }
    case 2: {
      uint32_t free = ESP.getFreeHeap();
      uint32_t total = ESP.getHeapSize();
      uint32_t pct = (total - free) * 100 / total;
      return "RAM:" + String(pct) + "% Free:" + String(free / 1024) + "KB";
    }
    default: return "";
  }
}
