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
  
  // Try local modem SNTP first, then external servers
  String gatewayIP = WiFi.gatewayIP().toString();
  
  // Validate gateway IP before using it
  if (gatewayIP == "0.0.0.0" || gatewayIP.length() == 0) {
    Serial.println("[NTP] WARNING: Gateway IP not available, using external servers only");
    gatewayIP = "192.168.1.1"; // Fallback to common gateway IP
  }
  
  const char* ntpServers[] = {
    gatewayIP.c_str(),   // Local modem SNTP (best option)
    "216.239.35.0",      // time.google.com
    "162.159.200.123",   // time.cloudflare.com  
    "129.6.15.28",       // time.nist.gov
    "pool.ntp.org"       // Keep domain as fallback
  };
  
  for (int serverIndex = 0; serverIndex < 5; serverIndex++) {
    if (serverIndex == 0) {
      Serial.printf("[NTP] Trying server %d: %s (Local Modem SNTP)\n", serverIndex + 1, ntpServers[serverIndex]);
    } else {
      Serial.printf("[NTP] Trying server %d: %s\n", serverIndex + 1, ntpServers[serverIndex]);
    }
    
    // Setup NTP client with current server
    setupNTPClientWithServer(ntpServers[serverIndex]);
    
    // Try multiple times to sync time
    int attempts = 0;
    while (attempts < 2) { // Reduced attempts per server
      Serial.printf("[NTP] Attempting sync with %s (attempt %d/2)...\n", ntpServers[serverIndex], attempts + 1);
      
      if (syncTime()) {
        initialized = true;
        lastSync = millis();
        Serial.printf("[NTP] ✅ Service initialized successfully with %s!\n", ntpServers[serverIndex]);
        return true;
      }
      attempts++;
      Serial.printf("[NTP] ❌ Sync attempt %d failed with %s\n", attempts, ntpServers[serverIndex]);
      if (attempts < 2) {
        delay(1000); // Shorter delay
      }
    }
    
    Serial.printf("[NTP] ⚠️ Server %s failed after 2 attempts, trying next...\n", ntpServers[serverIndex]);
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
  if (!timeClient) {
    Serial.println("[NTP] ERROR: No NTP client available!");
    return false;
  }
  
  Serial.println("[NTP] Syncing time...");
  
  // Add timeout and retry logic
  unsigned long startTime = millis();
  bool success = false;
  
  // Try up to 3 times with shorter timeouts
  for (int attempt = 0; attempt < 3; attempt++) {
    Serial.printf("[NTP] Sync attempt %d/3...\n", attempt + 1);
    
    if (timeClient->update()) {
      lastSync = millis();
      unsigned long syncTime = millis() - startTime;
      Serial.printf("[NTP] Time synced in %lums: %s\n", syncTime, getFormattedTime().c_str());
      success = true;
      break;
    } else {
      Serial.printf("[NTP] Sync attempt %d failed\n", attempt + 1);
      if (attempt < 2) {
        delay(1000); // Wait 1 second before retry
      }
    }
  }
  
  if (!success) {
    Serial.println("[NTP] ERROR: All sync attempts failed!");
    Serial.println("[NTP] Possible causes:");
    Serial.println("[NTP] - Modem/router blocking UDP port 123");
    Serial.println("[NTP] - Firewall blocking NTP traffic");
    Serial.println("[NTP] - ISP blocking NTP servers");
    Serial.println("[NTP] - Network connectivity issues");
  }
  
  return success;
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
    
    // Test UDP connectivity by sending a test packet
    IPAddress testIP(8, 8, 8, 8); // Google DNS for test
    if (ntpUDP->beginPacket(testIP, 53)) { // DNS port for test
      uint8_t testData[] = {'t', 'e', 's', 't'};
      ntpUDP->write(testData, 4);
      if (ntpUDP->endPacket()) {
        Serial.println("[NTP] UDP connectivity test: OK");
      } else {
        Serial.println("[NTP] UDP connectivity test: FAILED (this is normal for some networks)");
      }
    } else {
      Serial.println("[NTP] UDP connectivity test: SKIPPED (packet creation failed)");
    }
  } else {
    Serial.println("[NTP] WARNING: Failed to open UDP port 123");
    Serial.println("[NTP] This usually means the modem/router is blocking UDP");
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
