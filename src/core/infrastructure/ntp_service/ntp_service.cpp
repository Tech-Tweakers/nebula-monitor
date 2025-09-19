#include "core/infrastructure/ntp_service/ntp_service.h"
#include <WiFi.h>
#include "core/infrastructure/logger/logger.h"

// Static member definitions
WiFiUDP* NTPService::ntpUDP = nullptr;
NTPClient* NTPService::timeClient = nullptr;
bool NTPService::initialized = false;
unsigned long NTPService::lastSync = 0;

bool NTPService::initialize() {
  if (initialized) return true;
  
  Serial_println("[NTP] Initializing NTP service...");
  
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial_println("[NTP] ERROR: WiFi not connected!");
    return false;
  }
  
  // Try local modem SNTP first, then external servers
  String gatewayIP = WiFi.gatewayIP().toString();
  
  // Validate gateway IP before using it
  if (gatewayIP == "0.0.0.0" || gatewayIP.length() == 0) {
    Serial_println("[NTP] WARNING: Gateway IP not available, using external servers only");
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
      Serial_printf("[NTP] Trying server %d: %s (Local Modem SNTP)\n", serverIndex + 1, ntpServers[serverIndex]);
    } else {
      Serial_printf("[NTP] Trying server %d: %s\n", serverIndex + 1, ntpServers[serverIndex]);
    }
    
    // Setup NTP client with current server
    setupNTPClientWithServer(ntpServers[serverIndex]);
    
    // Try multiple times to sync time
    int attempts = 0;
    while (attempts < 2) { // Reduced attempts per server
      Serial_printf("[NTP] Attempting sync with %s (attempt %d/2)...\n", ntpServers[serverIndex], attempts + 1);
      
      if (syncTime()) {
        initialized = true;
        lastSync = millis();
        Serial_printf("[NTP] ✅ Service initialized successfully with %s!\n", ntpServers[serverIndex]);
        return true;
      }
      attempts++;
      Serial_printf("[NTP] ❌ Sync attempt %d failed with %s\n", attempts, ntpServers[serverIndex]);
      if (attempts < 2) {
        delay(1000); // Shorter delay
      }
    }
    
    Serial_printf("[NTP] ⚠️ Server %s failed after 2 attempts, trying next...\n", ntpServers[serverIndex]);
    delay(500); // Brief delay before next server
  }
  
  Serial_println("[NTP] ERROR: Failed to sync time with all servers!");
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
    Serial_println("[NTP] ERROR: No NTP client available!");
    return false;
  }
  
  Serial_println("[NTP] Syncing time...");
  
  // Add timeout and retry logic
  unsigned long startTime = millis();
  bool success = false;
  
  // Try up to 3 times with shorter timeouts
  for (int attempt = 0; attempt < 3; attempt++) {
    Serial_printf("[NTP] Sync attempt %d/3...\n", attempt + 1);
    
    if (timeClient->update()) {
      lastSync = millis();
      unsigned long syncTime = millis() - startTime;
      Serial_printf("[NTP] Time synced in %lums: %s\n", syncTime, getFormattedTime().c_str());
      success = true;
      break;
    } else {
      Serial_printf("[NTP] Sync attempt %d failed\n", attempt + 1);
      if (attempt < 2) {
        delay(1000); // Wait 1 second before retry
      }
    }
  }
  
  if (!success) {
    Serial_println("[NTP] ERROR: All sync attempts failed!");
    Serial_println("[NTP] Possible causes:");
    Serial_println("[NTP] - Modem/router blocking UDP port 123");
    Serial_println("[NTP] - Firewall blocking NTP traffic");
    Serial_println("[NTP] - ISP blocking NTP servers");
    Serial_println("[NTP] - Network connectivity issues");
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
      Serial_println("[NTP] Time seems invalid, attempting sync...");
      return syncTime();
    }
  }
  
  // No sync needed
  return true;
}

String NTPService::getCurrentTime() {
  // Try to initialize if not done yet (for late WiFi connection)
  if (!initialized && WiFi.status() == WL_CONNECTED) {
    Serial_println("[NTP] Late initialization attempt...");
    initialize();
  }
  
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

String NTPService::getCurrentDateTime() {
  // Late initialization attempt if not initialized but WiFi is connected
  if (!initialized && WiFi.status() == WL_CONNECTED) {
    Serial_println("[NTP] Late initialization attempt...");
    initialize();
  }
  
  if (!timeClient) {
    // Fallback to uptime format
    unsigned long uptime = millis() / 1000;
    unsigned long hours = uptime / 3600;
    unsigned long minutes = (uptime % 3600) / 60;
    unsigned long seconds = uptime % 60;
    
    String timeStr = "";
    if (hours < 10) timeStr += "0";
    timeStr += String(hours) + ":";
    if (minutes < 10) timeStr += "0";
    timeStr += String(minutes) + ":";
    if (seconds < 10) timeStr += "0";
    timeStr += String(seconds);
    
    return "Uptime: " + timeStr;
  }
  
  unsigned long epochTime = timeClient->getEpochTime();
  return formatDateTime(epochTime);
}

String NTPService::getFormattedTime() {
  if (!timeClient) {
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
  
  Serial_printf("[NTP] Configured: Server=%s, Offset=%d seconds, Timeout=10s\n", 
               server, timezoneOffset);
  
  // Test UDP connection
  if (ntpUDP->begin(123)) { // NTP port
    Serial_println("[NTP] UDP port 123 opened successfully");
    
    // Test UDP connectivity by sending a test packet
    IPAddress testIP(8, 8, 8, 8); // Google DNS for test
    if (ntpUDP->beginPacket(testIP, 53)) { // DNS port for test
      uint8_t testData[] = {'t', 'e', 's', 't'};
      ntpUDP->write(testData, 4);
      if (ntpUDP->endPacket()) {
        Serial_println("[NTP] UDP connectivity test: OK");
      } else {
        Serial_println("[NTP] UDP connectivity test: FAILED (this is normal for some networks)");
      }
    } else {
      Serial_println("[NTP] UDP connectivity test: SKIPPED (packet creation failed)");
    }
  } else {
    Serial_println("[NTP] WARNING: Failed to open UDP port 123");
    Serial_println("[NTP] This usually means the modem/router is blocking UDP");
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

String NTPService::formatDateTime(unsigned long epochTime) {
  // Convert epoch time to local time components
  unsigned long localTime = epochTime;
  
  // Calculate date components
  unsigned long days = localTime / 86400;
  unsigned long year = 1970 + (days / 365);
  unsigned long remainingDays = days % 365;
  
  // Simple month calculation (approximate)
  int month = 1;
  int day = 1;
  int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  
  for (int i = 0; i < 12; i++) {
    if (remainingDays <= daysInMonth[i]) {
      month = i + 1;
      day = remainingDays + 1;
      break;
    }
    remainingDays -= daysInMonth[i];
  }
  
  // Calculate time components
  unsigned long hours = (localTime % 86400) / 3600;
  unsigned long minutes = (localTime % 3600) / 60;
  unsigned long seconds = localTime % 60;
  
  // Format: DD/MM/YYYY HH:MM:SS
  String dateTimeStr = "";
  if (day < 10) dateTimeStr += "0";
  dateTimeStr += String(day) + "/";
  if (month < 10) dateTimeStr += "0";
  dateTimeStr += String(month) + "/" + String(year) + " ";
  if (hours < 10) dateTimeStr += "0";
  dateTimeStr += String(hours) + ":";
  if (minutes < 10) dateTimeStr += "0";
  dateTimeStr += String(minutes) + ":";
  if (seconds < 10) dateTimeStr += "0";
  dateTimeStr += String(seconds);
  
  return dateTimeStr;
}
