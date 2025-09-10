#pragma once
#include <Arduino.h>
#include <SPIFFS.h>
#include "core/infrastructure/sdcard_manager/sdcard_manager.h"

class ConfigLoader {
private:
  static bool initialized;
  static String configValues[50];
  static const char* configKeys[50];
  static int configCount;
  
  // Internal methods
  static void parseConfigLine(const String& line);
  static String getValue(const char* key, const String& defaultValue = "");
  
public:
  // Initialization
  static bool load();
  static void cleanup();
  
  // WiFi Configuration
  static String getWifiSSID();
  static String getWifiPassword();
  
  // Telegram Configuration
  static String getTelegramBotToken();
  static String getTelegramChatId();
  static bool isTelegramEnabled();
  
  // Alert Configuration
  static int getMaxFailuresBeforeAlert();
  static unsigned long getAlertCooldownMs();
  static unsigned long getAlertRecoveryCooldownMs();
  
  // Debug Configuration
  static bool isDebugLogsEnabled();
  static bool isAllLogsEnabled();
  
  // Network Targets
  static int getTargetCount();
  static String getTargetName(int index);
  static String getTargetUrl(int index);
  static String getTargetHealthEndpoint(int index);
  static String getTargetMonitorType(int index);
  
  // Display Configuration
  static int getDisplayRotation();
  static int getBacklightPin();
  
  // Touch Configuration
  static int getTouchSckPin();
  static int getTouchMosiPin();
  static int getTouchMisoPin();
  static int getTouchCsPin();
  static int getTouchIrqPin();
  static int getTouchXMin();
  static int getTouchXMax();
  static int getTouchYMin();
  static int getTouchYMax();
  
  // Performance Configuration
  static unsigned long getScanIntervalMs();
  static unsigned long getTouchFilterMs();
  static unsigned long getHttpTimeoutMs();
  
  // LED Configuration
  static int getLedPinR();
  static int getLedPinG();
  static int getLedPinB();
  static bool isLedActiveHigh();
  static int getLedPwmFreq();
  static int getLedPwmResBits();
  static int getLedBrightR();
  static int getLedBrightG();
  static int getLedBrightB();
  
  // NTP Configuration
  static int getTimezoneOffset();
  static String getNtpServer();
  
  // Health Check Configuration
  static String getHealthCheckHealthyPatterns();
  static String getHealthCheckUnhealthyPatterns();
  static bool isHealthCheckStrictMode();
  
  // Debug
  static void printAllConfigs();
  static bool isInitialized() { return initialized; }
};
