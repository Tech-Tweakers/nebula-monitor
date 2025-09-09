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
  
  // Try multiple NTP servers
  const char* ntpServers[] = {
    "pool.ntp.org",
    "time.google.com", 
    "time.cloudflare.com",
    "time.nist.gov"
  };
  
  for (int serverIndex = 0; serverIndex < 4; serverIndex++) {
    Serial.printf("[NTP] Trying server %d: %s\n", serverIndex + 1, ntpServers[serverIndex]);
    
    // Setup NTP client with current server
    setupNTPClientWithServer(ntpServers[serverIndex]);
    
    // Try multiple times to sync time
    int attempts = 0;
    while (attempts < 2) { // Reduced attempts per server
      if (syncTime()) {
        initialized = true;
        lastSync = millis();
        Serial.printf("[NTP] Service initialized successfully with %s!\n", ntpServers[serverIndex]);
        return true;
      }
      attempts++;
      Serial.printf("[NTP] Sync attempt %d failed, retrying...\n", attempts);
      delay(1000); // Shorter delay
    }
    
    Serial.printf("[NTP] Server %s failed, trying next...\n", ntpServers[serverIndex]);
    delay(500); // Brief delay before next server
  }
  
  Serial.println("[NTP] ERROR: Failed to sync time with all servers!");
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

bool NTPService::syncTimeIfNeeded() {
  if (!initialized || !timeClient) {
    return false;
  }
  
  unsigned long now = millis();
  
  // Check if we need to sync based on interval
  if (now - lastSync > SYNC_INTERVAL_MS) {
    return syncTime();
  }
  
  // Check if time seems invalid (before 2020 or not set)
  if (!timeClient->isTimeSet() || timeClient->getEpochTime() < 1600000000) {
    // Only sync if minimum interval has passed to avoid spam
    if (now - lastSync > MIN_SYNC_INTERVAL_MS) {
      Serial.println("[NTP] Time seems invalid, attempting sync...");
      return syncTime();
    }
  }
  
  // No sync needed
  return true;
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
  
  // Use smart sync - only sync if really needed
  syncTimeIfNeeded();
  
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
  setupNTPClientWithServer(ntpServer.c_str());
}

void NTPService::setupNTPClientWithServer(const char* server) {
  // Clean up existing client
  if (timeClient) {
    delete timeClient;
    timeClient = nullptr;
  }
  if (ntpUDP) {
    delete ntpUDP;
    ntpUDP = nullptr;
  }
  
  // Get timezone offset
  int timezoneOffset = ConfigLoader::getTimezoneOffset();
  
  // Create UDP and NTP client with shorter timeout
  ntpUDP = new WiFiUDP();
  timeClient = new NTPClient(*ntpUDP, server, timezoneOffset, 10000); // 10 second timeout
  
  Serial.printf("[NTP] Configured: Server=%s, Offset=%d seconds, Timeout=10s\n", 
               server, timezoneOffset);
  
  // Test UDP connection
  if (ntpUDP->begin(123)) { // NTP port
    Serial.println("[NTP] UDP port 123 opened successfully");
  } else {
    Serial.println("[NTP] WARNING: Failed to open UDP port 123");
  }
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
