#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>

// Core includes
#include "core/domain/status.h"
#include "core/application/network_monitor.h"
#include "core/infrastructure/wifi_service.h"
#include "core/infrastructure/http_client.h"
#include "core/infrastructure/telegram_service.h"
#include "ui/display_manager.h"
#include "ui/touch_handler.h"
#include "ui/led_controller.h"
#include "tasks/task_manager.h"
#include "config/config_loader.h"

// Global instances
WiFiService* wifiService;
HttpClient* httpClient;
TelegramService* telegramService;
DisplayManager* displayManager;
NetworkMonitor* networkMonitor;
TaskManager* taskManager;

// LVGL display objects
TFT_eSPI* tft;
lv_disp_draw_buf_t draw_buf;
lv_color_t buf1[240 * 10];
lv_color_t buf2[240 * 10];
lv_disp_drv_t disp_drv;

// LVGL flush callback
void display_flush(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  
  tft->startWrite();
  tft->setAddrWindow(area->x1, area->y1, w, h);
  tft->pushColors((uint16_t*)&color_p->full, w * h, true);
  tft->endWrite();
  
  lv_disp_flush_ready(disp);
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("========================================");
  Serial.println("    NEBULA MONITOR v2.4 - CLEAN ARCH");
  Serial.println("========================================");
  
  // 1. Load configuration
  Serial.println("[MAIN] Loading configuration...");
  if (!ConfigLoader::load()) {
    Serial.println("[MAIN] ERROR: Failed to load configuration!");
    return;
  }
  Serial.println("[MAIN] Configuration loaded successfully!");
  
  // 2. Initialize TFT display
  Serial.println("[MAIN] Initializing TFT display...");
  tft = new TFT_eSPI();
  tft->init();
  tft->setRotation(2);
  tft->fillScreen(TFT_BLACK);
  
  // Set backlight
  pinMode(27, OUTPUT);
  digitalWrite(27, HIGH);
  
  // 3. Initialize LVGL
  Serial.println("[MAIN] Initializing LVGL...");
  lv_init();
  
  // Initialize display buffer
  lv_disp_draw_buf_init(&draw_buf, buf1, buf2, 240 * 10);
  
  // Initialize display driver
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = 240;
  disp_drv.ver_res = 320;
  disp_drv.flush_cb = display_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);
  
  // 4. Create service instances
  Serial.println("[MAIN] Creating service instances...");
  wifiService = new WiFiService();
  httpClient = new HttpClient();
  telegramService = new TelegramService();
  displayManager = new DisplayManager();
  networkMonitor = new NetworkMonitor();
  taskManager = new TaskManager();
  
  // 5. Configure dependencies
  Serial.println("[MAIN] Configuring dependencies...");
  networkMonitor->setDependencies(wifiService, httpClient, telegramService, displayManager, taskManager);
  taskManager->setDependencies(networkMonitor, displayManager);
  
  // 6. Initialize services
  Serial.println("[MAIN] Initializing services...");
  
  // Initialize WiFi
  String ssid = ConfigLoader::getWifiSSID();
  String password = ConfigLoader::getWifiPassword();
  if (!wifiService->initialize(ssid, password)) {
    Serial.println("[MAIN] WARNING: WiFi initialization failed, will retry...");
  }
  
  // Initialize Telegram
  String botToken = ConfigLoader::getTelegramBotToken();
  String chatId = ConfigLoader::getTelegramChatId();
  bool telegramEnabled = ConfigLoader::isTelegramEnabled();
  if (telegramService->initialize(botToken, chatId, telegramEnabled)) {
    Serial.println("[MAIN] Telegram service initialized!");
  } else {
    Serial.println("[MAIN] Telegram service not available");
  }
  
  // Initialize display manager
  if (!displayManager->initialize()) {
    Serial.println("[MAIN] ERROR: Failed to initialize display manager!");
    return;
  }
  
  // Initialize network monitor
  if (!networkMonitor->initialize()) {
    Serial.println("[MAIN] ERROR: Failed to initialize network monitor!");
    return;
  }
  
  // Set targets for display
  displayManager->setTargets(networkMonitor->getTargets(), networkMonitor->getTargetCount());
  
  // Send test message with targets if Telegram is active and targets are loaded
  if (telegramService->isActive()) {
    int targetCount = networkMonitor->getTargetCount();
    if (targetCount > 0) {
      String targetNames[10];
      for (int i = 0; i < targetCount; i++) {
        targetNames[i] = networkMonitor->getTargets()[i].getName();
      }
      telegramService->sendTestMessage(targetNames, targetCount);
    } else {
      Serial.println("[MAIN] WARNING: No targets loaded, skipping Telegram test message");
    }
  }
  
  // Initialize LED controller
  if (!LEDController::initialize()) {
    Serial.println("[MAIN] WARNING: Failed to initialize LED controller!");
  }
  
  // Initialize touch handler
  if (!TouchHandler::initialize()) {
    Serial.println("[MAIN] WARNING: Failed to initialize touch handler!");
  }
  
  // 7. Initialize task manager
  if (!taskManager->initialize()) {
    Serial.println("[MAIN] ERROR: Failed to initialize task manager!");
    return;
  }
  
  // 8. Start tasks
  Serial.println("[MAIN] Starting FreeRTOS tasks...");
  if (!taskManager->startTasks()) {
    Serial.println("[MAIN] ERROR: Failed to start tasks!");
    return;
  }
  
  Serial.println("========================================");
  Serial.println("    SYSTEM INITIALIZED SUCCESSFULLY!");
  Serial.println("========================================");
  Serial.printf("Targets: %d\n", networkMonitor->getTargetCount());
  Serial.printf("WiFi: %s\n", wifiService->isConnected() ? "Connected" : "Disconnected");
  Serial.printf("Telegram: %s\n", telegramService->isActive() ? "Active" : "Inactive");
  Serial.println("========================================");
}

void loop() {
  // All work is done by FreeRTOS tasks
  // This loop just provides a heartbeat
  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat >= 30000) { // Every 30 seconds
    Serial.println("[MAIN] System running...");
    lastHeartbeat = millis();
  }
  
  delay(1000);
}
