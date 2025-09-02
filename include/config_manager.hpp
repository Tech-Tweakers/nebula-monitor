#pragma once
#include <Arduino.h>
#include <SPIFFS.h>

class ConfigManager {
private:
  static bool initialized;
  static String configValues[50]; // Array para armazenar valores
  static const char* configKeys[50]; // Array para chaves
  static int configCount;
  
  static void parseConfigLine(const String& line);
  static String getValue(const char* key, const String& defaultValue = "");
  
public:
  static bool begin();
  static void end();
  
  // WiFi
  static String getWifiSSID();
  static String getWifiPass();
  
  // Telegram
  static String getTelegramBotToken();
  static String getTelegramChatId();
  static bool isTelegramEnabled();
  
  // Alerts
  static int getMaxFailuresBeforeAlert();
  static unsigned long getAlertCooldownMs();
  static unsigned long getAlertRecoveryCooldownMs();
  
  // Debug
  static bool isDebugLogsEnabled();
  static bool isTouchLogsEnabled();
  static bool isAllLogsEnabled();
  
  // Network Targets
  static int getTargetCount();
  static String getTargetName(int index);
  static String getTargetUrl(int index);
  static String getTargetHealthEndpoint(int index);
  static String getTargetMonitorType(int index);
  
  // Display
  static int getDisplayRotation();
  static int getBacklightPin();
  
  // Touch
  static int getTouchSckPin();
  static int getTouchMosiPin();
  static int getTouchMisoPin();
  static int getTouchCsPin();
  static int getTouchIrqPin();
  static int getTouchXMin();
  static int getTouchXMax();
  static int getTouchYMin();
  static int getTouchYMax();
  
  // Performance
  static unsigned long getScanIntervalMs();
  static unsigned long getTouchFilterMs();
  static unsigned long getHttpTimeoutMs();
  
  // Debug
  static void printAllConfigs();
};
