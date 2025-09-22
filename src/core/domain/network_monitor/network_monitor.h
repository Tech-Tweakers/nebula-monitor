#pragma once
#include "core/domain/target/target.h"
#include "core/domain/alert/alert.h"
#include "core/domain/config/constants.h"
#include "core/domain/config/target_config.h"
#include "core/infrastructure/wifi_service/wifi_service.h"
#include "core/infrastructure/http_client/http_client.h"
#include "core/infrastructure/telegram_service/telegram_service.h"
#include "ui/display_manager/display_manager.h"
#include "core/infrastructure/task_manager/task_manager.h"
#include <Arduino.h>

class NetworkMonitor {
private:
  // Dependencies
  WiFiService* wifiService;
  HttpClient* httpClient;
  TelegramService* telegramService;
  DisplayManager* displayManager;
  TaskManager* taskManager;
  
  // State
  Target* targets;
  int targetCount;
  int maxTargets;
  bool scanning;
  unsigned long lastScanTime;
  unsigned long scanInterval;
  unsigned long scanStartTime;
  unsigned long lastScanDuration;
  
  // Configuration
  bool initialized;
  
public:
  NetworkMonitor();
  ~NetworkMonitor();
  
  // Initialization
  bool initialize();
  void setDependencies(WiFiService* wifi, HttpClient* http, TelegramService* telegram, 
                      DisplayManager* display, TaskManager* tasks);
  
  // Main operations
  void update();
  void startScanning();
  void stopScanning();
  
  // Target management
  bool loadTargets();
  bool allocateTargets();
  void deallocateTargets();
  void scanTarget(int index);
  void updateTargetStatus(int index, Status status, uint16_t latency);
  uint16_t performSafeHealthCheck(const String& url, const String& endpoint, uint16_t timeout = 10000);
  
  // Getters
  int getTargetCount() const { return targetCount; }
  int getMaxTargets() const { return maxTargets; }
  Target* getTargets() { return targets; }
  bool isScanning() const { return scanning; }
  bool isInitialized() const { return initialized; }
  
  // Configuration
  void setScanInterval(unsigned long interval) { scanInterval = interval; }
  unsigned long getScanInterval() const { return scanInterval; }
  
  // Performance and diagnostics
  void printPerformanceMetrics() const;
  void resetPerformanceMetrics();
  
  // Scan monitoring
  bool isScanStuck() const;
  void forceStopScan();
  unsigned long getLastScanDuration() const { return lastScanDuration; }
  
private:
  // Internal methods
  void processScanResults();
  void notifyDisplayUpdate(int index, Status status, uint16_t latency);
  MonitorType parseMonitorType(const String& type) const;
};
