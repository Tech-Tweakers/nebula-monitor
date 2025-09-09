#include "sdcard_manager.h"

// Static member definitions
const char* SDCardManager::CONFIG_FILENAME = "/config.env";
bool SDCardManager::initialized = false;
int SDCardManager::sdCsPin = DEFAULT_SD_CS_PIN;

SDCardManager& SDCardManager::getInstance() {
  static SDCardManager instance;
  return instance;
}

bool SDCardManager::initialize() {
  if (initialized) return true;
  
  Serial.println("[SDCARD] Initializing SDCard manager...");
  
  // Get SD CS pin from build flags
  #ifdef SD_CS
    sdCsPin = SD_CS;
  #else
    sdCsPin = DEFAULT_SD_CS_PIN;
  #endif
  
  Serial.printf("[SDCARD] Using SD CS pin: %d\n", sdCsPin);
  
  // Initialize SD Card
  if (!SD.begin(sdCsPin)) {
    Serial.println("[SDCARD] WARNING: SD Card not available");
    Serial.println("[SDCARD] System will use SPIFFS only");
    return false;
  }
  
  Serial.println("[SDCARD] SD Card initialized successfully!");
  initialized = true;
  
  // Try to sync configuration
  syncConfigFromSD();
  
  return true;
}

bool SDCardManager::isSDCardAvailable() {
  if (!initialized) {
    return SD.begin(sdCsPin);
  }
  return true;
}

bool SDCardManager::syncConfigFromSD() {
  if (!isSDCardAvailable()) {
    Serial.println("[SDCARD] SD Card not available for sync");
    return false;
  }
  
  // Check if config exists on SD
  if (!SD.exists(CONFIG_FILENAME)) {
    Serial.println("[SDCARD] No config.env found on SD Card");
    return false;
  }
  
  // Check if SD config is newer
  if (!isSDConfigNewer()) {
    Serial.println("[SDCARD] SD config is not newer, skipping sync");
    return false;
  }
  
  Serial.println("[SDCARD] SD config is newer, syncing to SPIFFS...");
  
  // Open SD file
  File sdFile = SD.open(CONFIG_FILENAME, FILE_READ);
  if (!sdFile) {
    Serial.println("[SDCARD] ERROR: Failed to open SD config file");
    return false;
  }
  
  // Open SPIFFS file for writing
  File spiffsFile = SPIFFS.open(CONFIG_FILENAME, FILE_WRITE);
  if (!spiffsFile) {
    Serial.println("[SDCARD] ERROR: Failed to open SPIFFS config file for writing");
    sdFile.close();
    return false;
  }
  
  // Copy file content
  bool success = copyFile(sdFile, spiffsFile);
  
  // Close files
  sdFile.close();
  spiffsFile.close();
  
  if (success) {
    Serial.println("[SDCARD] SUCCESS: Config synced from SD to SPIFFS!");
    Serial.println("[SDCARD] System will restart to load new configuration...");
    
    // Restart system to load new config
    delay(2000);
    ESP.restart();
  } else {
    Serial.println("[SDCARD] ERROR: Failed to sync config from SD");
  }
  
  return success;
}

bool SDCardManager::syncConfigToSD() {
  if (!isSDCardAvailable()) {
    Serial.println("[SDCARD] SD Card not available for sync");
    return false;
  }
  
  // Check if config exists on SPIFFS
  if (!SPIFFS.exists(CONFIG_FILENAME)) {
    Serial.println("[SDCARD] No config.env found on SPIFFS");
    return false;
  }
  
  Serial.println("[SDCARD] Syncing config from SPIFFS to SD...");
  
  // Open SPIFFS file
  File spiffsFile = SPIFFS.open(CONFIG_FILENAME, FILE_READ);
  if (!spiffsFile) {
    Serial.println("[SDCARD] ERROR: Failed to open SPIFFS config file");
    return false;
  }
  
  // Open SD file for writing
  File sdFile = SD.open(CONFIG_FILENAME, FILE_WRITE);
  if (!sdFile) {
    Serial.println("[SDCARD] ERROR: Failed to open SD config file for writing");
    spiffsFile.close();
    return false;
  }
  
  // Copy file content
  bool success = copyFile(spiffsFile, sdFile);
  
  // Close files
  spiffsFile.close();
  sdFile.close();
  
  if (success) {
    Serial.println("[SDCARD] SUCCESS: Config synced from SPIFFS to SD!");
  } else {
    Serial.println("[SDCARD] ERROR: Failed to sync config to SD");
  }
  
  return success;
}

bool SDCardManager::isSDConfigNewer() {
  if (!isSDCardAvailable() || !SD.exists(CONFIG_FILENAME)) {
    return false;
  }
  
  // Open SD file
  File sdFile = SD.open(CONFIG_FILENAME, FILE_READ);
  if (!sdFile) {
    return false;
  }
  
  // Open SPIFFS file
  File spiffsFile = SPIFFS.open(CONFIG_FILENAME, FILE_READ);
  if (!spiffsFile) {
    sdFile.close();
    return true; // SD exists but SPIFFS doesn't
  }
  
  // Compare modification times
  time_t sdTime = getFileModTime(sdFile);
  time_t spiffsTime = getFileModTime(spiffsFile);
  
  sdFile.close();
  spiffsFile.close();
  
  return sdTime > spiffsTime;
}

time_t SDCardManager::getFileModTime(File& file) {
  // For ESP32, we'll use file size as a proxy for modification time
  // In a real implementation, you'd use file.getLastWrite() if available
  return file.size();
}

bool SDCardManager::copyFile(File& source, File& destination) {
  if (!source || !destination) {
    return false;
  }
  
  size_t bytesRead = 0;
  size_t bytesWritten = 0;
  uint8_t buffer[512];
  
  while (source.available()) {
    size_t bytesToRead = source.read(buffer, sizeof(buffer));
    if (bytesToRead == 0) break;
    
    size_t bytesToWrite = destination.write(buffer, bytesToRead);
    if (bytesToWrite != bytesToRead) {
      Serial.println("[SDCARD] ERROR: Write mismatch during copy");
      return false;
    }
    
    bytesRead += bytesToRead;
    bytesWritten += bytesToWrite;
  }
  
  Serial.printf("[SDCARD] Copied %d bytes from source to destination\n", bytesWritten);
  return bytesWritten > 0;
}

void SDCardManager::cleanup() {
  if (initialized) {
    initialized = false;
    Serial.println("[SDCARD] SDCard manager cleaned up");
  }
}
