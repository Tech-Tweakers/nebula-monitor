#include "ntp_service.h"
#include <WiFi.h>

// Static member definitions
WiFiUDP* NTPService::ntpUDP = nullptr;
NTPClient* NTPService::timeClient = nullptr;
bool NTPService::initialized = false;
unsigned long NTPService::lastSync = 0;

bool NTPService::initialize() {
  if (initialized) return true;
  
  Serial.println("[NTP] Initializing NTP service...");
  
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[NTP] ERROR: WiFi not connected!");
    return false;
  }
  
  // Setup NTP client
  setupNTPClient();
  
  // Try multiple times to sync time
  int attempts = 0;
  while (attempts < 3) {
    if (syncTime()) {
      initialized = true;
      lastSync = millis();
      Serial.println("[NTP] Service initialized successfully!");
      return true;
    }
    attempts++;
    Serial.printf("[NTP] Sync attempt %d failed, retrying...\n", attempts);
    delay(2000); // Wait 2 seconds before retry
  }
  
  Serial.println("[NTP] ERROR: Failed to sync time after 3 attempts!");
  return false;
}

void NTPService::cleanup() {
  if (timeClient) {
    delete timeClient;
    timeClient = nullptr;
  }
  
  if (ntpUDP) {
    delete ntpUDP;
    ntpUDP = nullptr;
  }
  
  initialized = false;
  lastSync = 0;
}

bool NTPService::syncTime() {
  if (!initialized || !timeClient) {
    return false;
  }
  
  Serial.println("[NTP] Syncing time...");
  
  if (timeClient->update()) {
    lastSync = millis();
    Serial.printf("[NTP] Time synced: %s\n", getFormattedTime().c_str());
    return true;
  } else {
    Serial.println("[NTP] ERROR: Time sync failed!");
    return false;
  }
}

String NTPService::getCurrentTime() {
  if (!initialized || !timeClient) {
    // Fallback to uptime if NTP not available
    unsigned long now = millis();
    unsigned long hours = (now / 3600000) % 24;
    unsigned long minutes = (now / 60000) % 60;
    unsigned long seconds = (now / 1000) % 60;
    
    String timeStr = "";
    if (hours < 10) timeStr += "0";
    timeStr += String(hours) + ":";
    if (minutes < 10) timeStr += "0";
    timeStr += String(minutes) + ":";
    if (seconds < 10) timeStr += "0";
    timeStr += String(seconds);
    
    return timeStr + " (uptime)";
  }
  
  // Check if we need to sync
  if (millis() - lastSync > SYNC_INTERVAL_MS) {
    syncTime();
  }
  
  // Check if time is actually set and valid
  if (!timeClient->isTimeSet() || timeClient->getEpochTime() < 1600000000) { // Before 2020
    // Try to sync one more time
    if (syncTime()) {
      unsigned long epochTime = timeClient->getEpochTime();
      return formatTime(epochTime);
    } else {
      // Fallback to uptime if sync fails
      unsigned long now = millis();
      unsigned long hours = (now / 3600000) % 24;
      unsigned long minutes = (now / 60000) % 60;
      unsigned long seconds = (now / 1000) % 60;
      
      String timeStr = "";
      if (hours < 10) timeStr += "0";
      timeStr += String(hours) + ":";
      if (minutes < 10) timeStr += "0";
      timeStr += String(minutes) + ":";
      if (seconds < 10) timeStr += "0";
      timeStr += String(seconds);
      
      return timeStr + " (uptime)";
    }
  }
  
  unsigned long epochTime = timeClient->getEpochTime();
  return formatTime(epochTime);
}

String NTPService::getFormattedTime() {
  if (!initialized || !timeClient) {
    return "NTP not available";
  }
  
  unsigned long epochTime = timeClient->getEpochTime();
  return formatTime(epochTime);
}

bool NTPService::isTimeSynced() {
  return initialized && timeClient && timeClient->isTimeSet();
}

void NTPService::setupNTPClient() {
  // Get configuration
  String ntpServer = ConfigLoader::getNtpServer();
  int timezoneOffset = ConfigLoader::getTimezoneOffset();
  
  // Create UDP and NTP client
  ntpUDP = new WiFiUDP();
  timeClient = new NTPClient(*ntpUDP, ntpServer.c_str(), timezoneOffset, 60000);
  
  Serial.printf("[NTP] Configured: Server=%s, Offset=%d seconds\n", 
               ntpServer.c_str(), timezoneOffset);
}

String NTPService::formatTime(unsigned long epochTime) {
  // Convert epoch time to local time components
  unsigned long localTime = epochTime;
  
  unsigned long hours = (localTime % 86400) / 3600;
  unsigned long minutes = (localTime % 3600) / 60;
  unsigned long seconds = localTime % 60;
  
  String timeStr = "";
  if (hours < 10) timeStr += "0";
  timeStr += String(hours) + ":";
  if (minutes < 10) timeStr += "0";
  timeStr += String(minutes) + ":";
  if (seconds < 10) timeStr += "0";
  timeStr += String(seconds);
  
  return timeStr;
}
