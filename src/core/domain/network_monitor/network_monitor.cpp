#include "core/domain/network_monitor/network_monitor.h"
#include "config/config_loader/config_loader.h"
#include "core/infrastructure/memory_manager/memory_manager.h"
#include <Arduino.h>
#include "core/infrastructure/logger/logger.h"

NetworkMonitor::NetworkMonitor() 
  : wifiService(nullptr), httpClient(nullptr), telegramService(nullptr),
    displayManager(nullptr), taskManager(nullptr), targetCount(0), 
    scanning(false), lastScanTime(0), scanInterval(30000), initialized(false) {
}

NetworkMonitor::~NetworkMonitor() {
  // Dependencies are managed externally
}

bool NetworkMonitor::initialize() {
  if (initialized) return true;
  
  Serial_println("[NETWORK_MONITOR] Initializing...");
  
  // Load targets from configuration
  if (!loadTargets()) {
    Serial_println("[NETWORK_MONITOR] ERROR: Failed to load targets!");
    return false;
  }
  
  // Initialize WiFi if not already done
  if (wifiService && !wifiService->isConnected()) {
    String ssid = ConfigLoader::getWifiSSID();
    String password = ConfigLoader::getWifiPassword();
    wifiService->initialize(ssid, password);
  }
  
  // Initialize Telegram service
  if (telegramService) {
    String botToken = ConfigLoader::getTelegramBotToken();
    String chatId = ConfigLoader::getTelegramChatId();
    bool enabled = ConfigLoader::isTelegramEnabled();
    telegramService->initialize(botToken, chatId, enabled);
  }
  
  initialized = true;
  Serial_printf("[NETWORK_MONITOR] Initialized with %d targets\n", targetCount);
  
  return true;
}

void NetworkMonitor::setDependencies(WiFiService* wifi, HttpClient* http, TelegramService* telegram, 
                                    DisplayManager* display, TaskManager* tasks) {
  wifiService = wifi;
  httpClient = http;
  telegramService = telegram;
  displayManager = display;
  taskManager = tasks;
}

void NetworkMonitor::update() {
  if (!initialized) return;
  
  // Update WiFi service
  if (wifiService) {
    wifiService->update();
  }
  
  // Check if it's time to scan
  unsigned long now = millis();
  if (now - lastScanTime >= scanInterval) {
    startScanning();
  }
  
  // Process any ongoing scan
  if (scanning) {
    processScanResults();
  }
}

void NetworkMonitor::startScanning() {
  if (scanning) {
    Serial_println("[NETWORK_MONITOR] WARNING: Scan already in progress, skipping");
    return;
  }
  
  scanning = true;
  lastScanTime = millis();
  scanStartTime = millis();
  
  Serial_println("[NETWORK_MONITOR] Starting scan cycle...");
  
  // Notify display that scan started
  if (displayManager) {
    displayManager->onScanStarted();
  }
  
  // Scan all targets with timeout protection
  for (int i = 0; i < targetCount; i++) {
    unsigned long targetStartTime = millis();
    
    // Feed watchdog at start of each target
    MemoryManager::getInstance().feedWatchdog();
    
    // Check if scan is taking too long (30 seconds max per scan)
    if (millis() - scanStartTime > 30000) {
      Serial_println("[NETWORK_MONITOR] WARNING: Scan timeout, stopping remaining targets");
      break;
    }
    
    // Check memory before each target
    if (MemoryManager::getInstance().isMemoryCritical()) {
      Serial_println("[NETWORK_MONITOR] WARNING: Critical memory, stopping scan");
      break;
    }
    
    scanTarget(i);
    
    // Feed watchdog after each target
    MemoryManager::getInstance().feedWatchdog();
    
    // Check if this target took too long (10 seconds max per target)
    unsigned long targetDuration = millis() - targetStartTime;
    if (targetDuration > 10000) {
      Serial_printf("[NETWORK_MONITOR] WARNING: Target %d took %lums (too long)\n", i, targetDuration);
    }
    
    delay(200); // Small delay between targets
  }
  
  // Mark scan as complete
  scanning = false;
  
  lastScanDuration = millis() - scanStartTime;
  Serial_printf("[NETWORK_MONITOR] Scan cycle complete in %lums\n", lastScanDuration);
  
  // Notify display that scan completed
  if (displayManager) {
    displayManager->onScanCompleted();
  }
}

void NetworkMonitor::stopScanning() {
  scanning = false;
  Serial_println("[NETWORK_MONITOR] Scanning stopped");
}

bool NetworkMonitor::loadTargets() {
  targetCount = ConfigLoader::getTargetCount();
  
  if (targetCount == 0) {
    Serial_println("[NETWORK_MONITOR] No targets configured, using defaults");
    // Set up default targets
    targetCount = 6;
    targets[0] = Target("Proxmox HV", "http://192.168.1.128:8006/", "", PING);
    targets[1] = Target("Router #1", "http://192.168.1.1", "", PING);
    targets[2] = Target("Router #2", "https://192.168.1.172", "", PING);
    targets[3] = Target("Polaris API", "https://pet-chem-independence-australia.trycloudflare.com", "/health", HEALTH_CHECK);
    targets[4] = Target("Polaris INT", "http://ebfc52323306.ngrok-free.app", "/health", PING);
    targets[5] = Target("Polaris WEB", "https://tech-tweakers.github.io/polaris-v2-web", "", PING);
    return true;
  }
  
  // Load targets from configuration
  for (int i = 0; i < targetCount; i++) {
    String name = ConfigLoader::getTargetName(i);
    String url = ConfigLoader::getTargetUrl(i);
    String healthEndpoint = ConfigLoader::getTargetHealthEndpoint(i);
    String monitorTypeStr = ConfigLoader::getTargetMonitorType(i);
    
    MonitorType type = parseMonitorType(monitorTypeStr);
    
    targets[i] = Target(name, url, healthEndpoint, type);
    targets[i].setStatus(UNKNOWN);
    targets[i].setLatency(0);
    
    Serial_printf("[NETWORK_MONITOR] Target %d: %s | %s | %s | %s\n", 
                 i + 1, name.c_str(), url.c_str(), 
                 healthEndpoint.length() > 0 ? healthEndpoint.c_str() : "null",
                 monitorTypeStr.c_str());
  }
  
  return true;
}

void NetworkMonitor::scanTarget(int index) {
  if (index < 0 || index >= targetCount || !httpClient) {
    Serial_printf("[NETWORK_MONITOR] ERROR: Invalid scan target %d\n", index);
    return;
  }
  
  Target& target = targets[index];
  unsigned long targetStartTime = millis();
  
  // Optimized string handling
  const char* name = target.getName().c_str();
  const char* typeStr = (target.getMonitorType() == HEALTH_CHECK) ? "HEALTH_CHECK" : "PING";
  
  Serial_printf("[NETWORK_MONITOR] Checking %s (type: %s)...\n", name, typeStr);
  
  uint16_t latency = 0;
  
  // Check memory before proceeding
  if (MemoryManager::getInstance().isMemoryCritical()) {
    Serial_println("[NETWORK_MONITOR] ERROR: Critical memory, skipping target");
    updateTargetStatus(index, DOWN, 0);
    return;
  }
  
  // Reduced delay for better performance
  vTaskDelay(pdMS_TO_TICKS(50));
  
  // Feed watchdog before HTTP request
  MemoryManager::getInstance().feedWatchdog();
  
  // Timeout de 10s para conexões lentas, mas evita travamentos
  uint16_t timeout = 10000;
  
  // Perform the check with timeout protection
  if (target.getMonitorType() == HEALTH_CHECK) {
    // Use enhanced health check with timeout
    latency = performSafeHealthCheck(target.getUrl(), target.getHealthEndpoint(), timeout);
  } else {
    // Enhanced ping with timeout
    latency = httpClient->ping(target.getUrl(), timeout);
  }
  
  // Feed watchdog after HTTP request
  MemoryManager::getInstance().feedWatchdog();
  
  // Check if this target took too long
  unsigned long targetDuration = millis() - targetStartTime;
  if (targetDuration > 11000) { // 11s = timeout + margem
    Serial_printf("[NETWORK_MONITOR] WARNING: Target %s took %lums (timeout)\n", name, targetDuration);
  }
  
  // Fixed strategy: timeout and failures should be DOWN for proper alerting
  Status newStatus;
  if (latency > 0) {
    newStatus = UP;
  } else if (targetDuration > 11000) {
    // CRITICAL FIX: Timeout should be DOWN to trigger alerts, not UNKNOWN
    newStatus = DOWN;
    Serial_printf("[NETWORK_MONITOR] Target %s marked as DOWN due to timeout (%lums)\n", name, targetDuration);
  } else {
    newStatus = DOWN;
  }
  
  updateTargetStatus(index, newStatus, latency);
}

void NetworkMonitor::updateTargetStatus(int index, Status status, uint16_t latency) {
  if (index < 0 || index >= targetCount) return;
  
  Target& target = targets[index];
  target.setStatus(status);
  target.setLatency(latency);
  
  Serial_printf("[NETWORK_MONITOR] updateTargetStatus: %s: %s (%d ms)\n", 
               target.getName().c_str(), 
               target.getStatusText().c_str(), 
               latency);
  
  // Notify Telegram service
  if (telegramService) {
    telegramService->updateTargetStatus(index, status, latency, target.getName());
  }
  
  // Notify display
  notifyDisplayUpdate(index, status, latency);
}

void NetworkMonitor::processScanResults() {
  // This method can be used for post-processing scan results
  // For now, it's handled in updateTargetStatus
}

uint16_t NetworkMonitor::performSafeHealthCheck(const String& url, const String& endpoint, uint16_t timeout) {
  if (!httpClient) return 0;
  
  // Enhanced URL safety checks
  if (url.length() > 200) {
    Serial_println("[NETWORK_MONITOR] ERROR: URL too long for health check");
    return 0;
  }
  
  Serial_printf("[NETWORK_MONITOR] Performing enhanced health check: %s%s\n", url.c_str(), endpoint.c_str());
  
  // Use the enhanced health check with intelligent timeout and retry logic
  uint16_t latency = httpClient->healthCheck(url, endpoint, 0); // 0 = auto-calculate timeout
  
  if (latency > 0) {
    // Get the response payload for verification
    String response = httpClient->getLastResponse();
    int httpCode = httpClient->getLastHttpCode();
    
    if (response.length() > 0 && response.length() < 1000) {
      // Enhanced response validation
      if (httpClient->isHealthyResponse(response)) {
        Serial_printf("[NETWORK_MONITOR] Health check successful: %d ms (HTTP %d)\n", latency, httpCode);
      } else {
        Serial_printf("[NETWORK_MONITOR] Health check FAILED: Unhealthy response detected (HTTP %d)\n", httpCode);
        Serial_printf("[NETWORK_MONITOR] Response: %s\n", response.c_str());
        return 0;
      }
    } else {
      Serial_printf("[NETWORK_MONITOR] Health check successful: %d ms (HTTP %d, no response validation)\n", latency, httpCode);
    }
  } else {
    // Check error category for better logging
    ErrorCategory errorCategory = httpClient->getLastErrorCategory();
    int httpCode = httpClient->getLastHttpCode();
    
    switch (errorCategory) {
      case ErrorCategory::SSL_ERROR:
        Serial_printf("[NETWORK_MONITOR] Health check failed: SSL/TLS error (HTTP %d)\n", httpCode);
        break;
      case ErrorCategory::TEMPORARY:
        Serial_printf("[NETWORK_MONITOR] Health check failed: Temporary network issue (HTTP %d)\n", httpCode);
        break;
      case ErrorCategory::PERMANENT:
        Serial_printf("[NETWORK_MONITOR] Health check failed: Permanent error (HTTP %d)\n", httpCode);
        break;
      default:
        Serial_printf("[NETWORK_MONITOR] Health check failed: Unknown error (HTTP %d)\n", httpCode);
        break;
    }
    
    // Log response details for debugging
    String response = httpClient->getLastResponse();
    if (response.length() > 0) {
      Serial_printf("[NETWORK_MONITOR] Response: %s\n", response.c_str());
    }
  }
  
  return latency;
}

void NetworkMonitor::notifyDisplayUpdate(int index, Status status, uint16_t latency) {
  Serial_printf("[NETWORK_MONITOR] notifyDisplayUpdate: index=%d, status=%d, latency=%d\n", 
               index, status, latency);
  
  if (displayManager) {
    displayManager->updateTargetStatus(index, status, latency);
  } else {
    Serial_println("[NETWORK_MONITOR] ERROR: displayManager is null!");
  }
}

MonitorType NetworkMonitor::parseMonitorType(const String& type) const {
  if (type.equalsIgnoreCase("HEALTH_CHECK")) {
    return HEALTH_CHECK;
  }
  return PING;
}

void NetworkMonitor::printPerformanceMetrics() const {
  Serial_println("\n=== NETWORK MONITOR PERFORMANCE ===");
  Serial_printf("Targets: %d\n", targetCount);
  Serial_printf("Scanning: %s\n", scanning ? "YES" : "NO");
  Serial_printf("Scan Interval: %lu ms\n", scanInterval);
  Serial_printf("Last Scan: %lu ms ago\n", millis() - lastScanTime);
  
  if (httpClient) {
    Serial_println("\n--- HTTP Client Metrics ---");
    httpClient->printMetrics();
  }
  
  Serial_println("===============================\n");
}

void NetworkMonitor::resetPerformanceMetrics() {
  if (httpClient) {
    httpClient->resetMetrics();
  }
  Serial_println("[NETWORK_MONITOR] Performance metrics reset");
}

bool NetworkMonitor::isScanStuck() const {
  if (!scanning) return false;
  
  // If scan has been running for more than 60 seconds, consider it stuck
  unsigned long scanDuration = millis() - scanStartTime;
  return scanDuration > 60000;
}

void NetworkMonitor::forceStopScan() {
  if (!scanning) return;
  
  Serial_println("[NETWORK_MONITOR] EMERGENCY: Force stopping stuck scan!");
  scanning = false;
  
  // Notify display that scan was force-stopped
  if (displayManager) {
    displayManager->onScanCompleted();
  }
}
