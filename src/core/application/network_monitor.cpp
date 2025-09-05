#include "core/application/network_monitor.h"
#include "config/config_loader.h"
#include <Arduino.h>

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
  
  Serial.println("[NETWORK_MONITOR] Initializing...");
  
  // Load targets from configuration
  if (!loadTargets()) {
    Serial.println("[NETWORK_MONITOR] ERROR: Failed to load targets!");
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
  Serial.printf("[NETWORK_MONITOR] Initialized with %d targets\n", targetCount);
  
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
  if (scanning) return;
  
  scanning = true;
  lastScanTime = millis();
  
  Serial.println("[NETWORK_MONITOR] Starting scan cycle...");
  
  // Notify display that scan started
  if (displayManager) {
    displayManager->onScanStarted();
  }
  
  // Scan all targets
  for (int i = 0; i < targetCount; i++) {
    scanTarget(i);
    delay(200); // Small delay between targets
  }
  
  // Mark scan as complete
  scanning = false;
  
  Serial.println("[NETWORK_MONITOR] Scan cycle complete");
  
  // Notify display that scan completed
  if (displayManager) {
    displayManager->onScanCompleted();
  }
}

void NetworkMonitor::stopScanning() {
  scanning = false;
  Serial.println("[NETWORK_MONITOR] Scanning stopped");
}

bool NetworkMonitor::loadTargets() {
  targetCount = ConfigLoader::getTargetCount();
  
  if (targetCount == 0) {
    Serial.println("[NETWORK_MONITOR] No targets configured, using defaults");
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
    
    Serial.printf("[NETWORK_MONITOR] Target %d: %s | %s | %s | %s\n", 
                 i + 1, name.c_str(), url.c_str(), 
                 healthEndpoint.length() > 0 ? healthEndpoint.c_str() : "null",
                 monitorTypeStr.c_str());
  }
  
  return true;
}

void NetworkMonitor::scanTarget(int index) {
  if (index < 0 || index >= targetCount || !httpClient) return;
  
  Target& target = targets[index];
  
  // Limit string operations to prevent stack issues
  String name = target.getName();
  if (name.length() > 50) {
    name = name.substring(0, 50) + "...";
  }
  
  Serial.printf("[NETWORK_MONITOR] Checking %s (type: %s)...\n", 
               name.c_str(), 
               target.getMonitorType() == HEALTH_CHECK ? "HEALTH_CHECK" : "PING");
  
  uint16_t latency = 0;
  
  // Add small delay to prevent overwhelming the system
  vTaskDelay(pdMS_TO_TICKS(100));
  
  if (target.getMonitorType() == HEALTH_CHECK) {
    // Use safe health check with timeout and memory protection
    latency = performSafeHealthCheck(target.getUrl());
  } else {
    // Simple ping
    latency = httpClient->ping(target.getUrl());
  }
  
  Status newStatus = (latency > 0) ? UP : DOWN;
  updateTargetStatus(index, newStatus, latency);
}

void NetworkMonitor::updateTargetStatus(int index, Status status, uint16_t latency) {
  if (index < 0 || index >= targetCount) return;
  
  Target& target = targets[index];
  target.setStatus(status);
  target.setLatency(latency);
  
  Serial.printf("[NETWORK_MONITOR] updateTargetStatus: %s: %s (%d ms)\n", 
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

uint16_t NetworkMonitor::performSafeHealthCheck(const String& url) {
  if (!httpClient) return 0;
  
  // Limit URL length to prevent stack overflow
  String safeUrl = url;
  if (safeUrl.length() > 200) {
    safeUrl = safeUrl.substring(0, 200);
  }
  
  Serial.printf("[NETWORK_MONITOR] Performing robust health check: %s\n", safeUrl.c_str());
  
  // Use the robust health check with payload verification
  uint16_t latency = httpClient->healthCheck(safeUrl, "", 10000); // 10 second timeout
  
  if (latency > 0) {
    Serial.printf("[NETWORK_MONITOR] Health check successful: %d ms\n", latency);
    
    // Get the response payload for additional verification
    String response = httpClient->getLastResponse();
    if (response.length() > 0 && response.length() < 1000) {
      Serial.printf("[NETWORK_MONITOR] Response payload: %s\n", response.c_str());
      
      // Check for specific health indicators in the response
      if (response.indexOf("\"status\":\"healthy\"") > 0 || 
          response.indexOf("\"health\":\"ok\"") > 0 ||
          response.indexOf("\"status\":\"ok\"") > 0) {
        Serial.println("[NETWORK_MONITOR] Health status confirmed: healthy");
      } else {
        Serial.println("[NETWORK_MONITOR] Health status: response received but not explicitly healthy");
      }
    }
  } else {
    Serial.println("[NETWORK_MONITOR] Health check failed: No response");
  }
  
  return latency;
}

void NetworkMonitor::notifyDisplayUpdate(int index, Status status, uint16_t latency) {
  Serial.printf("[NETWORK_MONITOR] notifyDisplayUpdate: index=%d, status=%d, latency=%d\n", 
               index, status, latency);
  
  if (displayManager) {
    displayManager->updateTargetStatus(index, status, latency);
  } else {
    Serial.println("[NETWORK_MONITOR] ERROR: displayManager is null!");
  }
}

MonitorType NetworkMonitor::parseMonitorType(const String& type) const {
  if (type.equalsIgnoreCase("HEALTH_CHECK")) {
    return HEALTH_CHECK;
  }
  return PING;
}
