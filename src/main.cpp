#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>

// Core includes
#include "core/domain/status/status.h"
#include "core/domain/network_monitor/network_monitor.h"
#include "core/infrastructure/wifi_service/wifi_service.h"
#include "core/infrastructure/http_client/http_client.h"
#include "core/infrastructure/telegram_service/telegram_service.h"
#include "core/infrastructure/ssl_mutex_manager/ssl_mutex_manager.h"
#include "core/infrastructure/memory_manager/memory_manager.h"
#include "core/infrastructure/ntp_service/ntp_service.h"
#include "core/infrastructure/logger/logger.h"
#include "ui/display_manager/display_manager.h"
#include "ui/touch_handler/touch_handler.h"
#include "ui/led_controller/led_controller.h"
#include "core/infrastructure/task_manager/task_manager.h"
#include "config/config_loader/config_loader.h"

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
  
  LOG_LEGACY("========================================");
  LOG_LEGACY("    NEBULA MONITOR v2.5 - CLEAN ARCH");
  LOG_LEGACY("========================================");
  
  // 1. Load configuration
  Serial.println("[MAIN] Loading configuration...");
  if (!ConfigLoader::load()) {
    Serial.println("[ERROR] Failed to load configuration!");
    return;
  }
  Serial.println("[MAIN] Configuration loaded successfully!");
  
  // 1.5. Initialize logger interface after ConfigLoader is loaded
  ConfigLoader::initializeLoggerInterface();
  
  // 2. Initialize TFT display
  LOG_MAIN("Initializing TFT display...");
  tft = new TFT_eSPI();
  tft->init();
  tft->setRotation(2);
  tft->fillScreen(TFT_BLACK);
  
  // Set backlight
  pinMode(27, OUTPUT);
  digitalWrite(27, HIGH);
  
  // 3. Initialize LVGL
  LOG_MAIN("Initializing LVGL...");
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
  LOG_MAIN("Creating service instances...");
  wifiService = new WiFiService();
  httpClient = new HttpClient();
  telegramService = new TelegramService();
  displayManager = new DisplayManager();
  networkMonitor = new NetworkMonitor();
  taskManager = new TaskManager();
  
  // 5. Configure dependencies
  LOG_MAIN("Configuring dependencies...");
  networkMonitor->setDependencies(wifiService, httpClient, telegramService, displayManager, taskManager);
  taskManager->setDependencies(networkMonitor, displayManager);
  
  // 6. Initialize memory manager (Garbage Collection)
  LOG_MAIN("Initializing memory manager...");
  if (!MemoryManager::getInstance().initialize()) {
    LOG_ERROR("Failed to initialize memory manager!");
    return;
  }
  
  // 7. Initialize SSL mutex manager
  LOG_MAIN("Initializing SSL mutex manager...");
  if (!SSLMutexManager::initialize()) {
    LOG_ERROR("Failed to initialize SSL mutex manager!");
    return;
  }
  
  // 8. Initialize services
  LOG_MAIN("Initializing services...");
  
  // Initialize WiFi
  String ssid = ConfigLoader::getWifiSSID();
  String password = ConfigLoader::getWifiPassword();
  if (!wifiService->initialize(ssid, password)) {
    LOG_WARN("WiFi initialization failed, will retry...");
  }
  
  // Initialize NTP service (after WiFi)
  if (WiFi.status() == WL_CONNECTED) {
    if (NTPService::initialize()) {
      LOG_MAIN("NTP service initialized!");
    } else {
      LOG_WARN("NTP service failed to initialize!");
    }
  } else {
    LOG_WARN("WiFi not connected, skipping NTP initialization");
  }
  
  // Initialize Telegram
  String botToken = ConfigLoader::getTelegramBotToken();
  String chatId = ConfigLoader::getTelegramChatId();
  bool telegramEnabled = ConfigLoader::isTelegramEnabled();
  if (telegramService->initialize(botToken, chatId, telegramEnabled)) {
    LOG_MAIN("Telegram service initialized!");
  } else {
    LOG_MAIN("Telegram service not available");
  }
  
  // Initialize display manager
  if (!displayManager->initialize()) {
    LOG_ERROR("Failed to initialize display manager!");
    return;
  }
  
  // Initialize network monitor
  if (!networkMonitor->initialize()) {
    LOG_ERROR("Failed to initialize network monitor!");
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
      LOG_WARN("No targets loaded, skipping Telegram test message");
    }
  }
  
  // Initialize LED controller
  if (!LEDController::initialize()) {
    LOG_WARN("Failed to initialize LED controller!");
  }
  
  // Initialize touch handler
  if (!TouchHandler::initialize()) {
    LOG_WARN("Failed to initialize touch handler!");
  }
  
  // 9. Initialize task manager
  if (!taskManager->initialize()) {
    LOG_ERROR("Failed to initialize task manager!");
    return;
  }
  
  // 10. Start tasks
  LOG_MAIN("Starting FreeRTOS tasks...");
  if (!taskManager->startTasks()) {
    LOG_ERROR("Failed to start tasks!");
    return;
  }
  
  LOG_LEGACY("========================================");
  LOG_LEGACY("    SYSTEM INITIALIZED SUCCESSFULLY!");
  LOG_LEGACY("========================================");
  LOG_LEGACY_F("Targets: %d", networkMonitor->getTargetCount());
  LOG_LEGACY_F("WiFi: %s", wifiService->isConnected() ? "Connected" : "Disconnected");
  LOG_LEGACY_F("Telegram: %s", telegramService->isActive() ? "Active" : "Inactive");
  LOG_LEGACY("========================================");
}

void loop() {
  // All work is done by FreeRTOS tasks
  // This loop provides heartbeat and memory management
  static unsigned long lastHeartbeat = 0;
  static unsigned long lastMemoryCheck = 0;
  
  uint32_t now = millis();
  
  // Heartbeat every 30 seconds
  if (now - lastHeartbeat >= 30000) {
    LOG_MAIN("System running...");
    
    // Print memory stats with heartbeat
    MemoryManager::getInstance().printMemoryStats();
    
    lastHeartbeat = now;
  }
  
  // Memory check every 10 seconds
  if (now - lastMemoryCheck >= 10000) {
    MemoryManager::getInstance().handleMemoryPressure();
    lastMemoryCheck = now;
  }
  
  // Feed watchdog
  MemoryManager::getInstance().feedWatchdog();
  
  delay(1000);
}
