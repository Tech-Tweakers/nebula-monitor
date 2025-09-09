#pragma once
#include <Arduino.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include "config/config_loader.h"

class NTPService {
private:
  static WiFiUDP* ntpUDP;
  static NTPClient* timeClient;
  static bool initialized;
  static unsigned long lastSync;
  static const unsigned long SYNC_INTERVAL_MS = 300000; // 5 minutes
  static const unsigned long MIN_SYNC_INTERVAL_MS = 60000; // 1 minute minimum
  
public:
  // Initialization
  static bool initialize();
  static void cleanup();
  
  // Time management
  static bool syncTime();
  static bool syncTimeIfNeeded(); // Smart sync with minimum interval
  static String getCurrentTime();
  static String getFormattedTime();
  static bool isTimeSynced();
  
  // Status
  static bool isInitialized() { return initialized; }
  
private:
  // Internal methods
  static void setupNTPClient();
  static String formatTime(unsigned long epochTime);
};
