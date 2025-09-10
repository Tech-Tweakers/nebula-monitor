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
    Serial.println("[SDCARD] New configuration loaded successfully!");
    Serial.println("[SDCARD] No restart needed - config will be loaded on next scan cycle");
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
    Serial.println("[SDCARD] DEBUG: SD not available or config not found");
    return false;
  }
  
  // Open SD file
  File sdFile = SD.open(CONFIG_FILENAME, FILE_READ);
  if (!sdFile) {
    Serial.println("[SDCARD] DEBUG: Failed to open SD file");
    return false;
  }
  
  // Open SPIFFS file
  File spiffsFile = SPIFFS.open(CONFIG_FILENAME, FILE_READ);
  if (!spiffsFile) {
    Serial.println("[SDCARD] DEBUG: SPIFFS file not found, SD is newer");
    sdFile.close();
    return true; // SD exists but SPIFFS doesn't
  }
  
  // Compare file sizes first (quick check)
  size_t sdSize = sdFile.size();
  size_t spiffsSize = spiffsFile.size();
  
  Serial.printf("[SDCARD] DEBUG: File sizes - SD=%d, SPIFFS=%d\n", sdSize, spiffsSize);
  
  sdFile.close();
  spiffsFile.close();
  
  // If sizes are different, assume SD is newer if it's larger
  if (sdSize != spiffsSize) {
    Serial.printf("[SDCARD] Size difference: SD=%d, SPIFFS=%d\n", sdSize, spiffsSize);
    bool isNewer = sdSize > spiffsSize;
    Serial.printf("[SDCARD] DEBUG: SD is %s based on size\n", isNewer ? "newer" : "older");
    return isNewer;
  }
  
  // If sizes are the same, compare content hash
  Serial.println("[SDCARD] DEBUG: Same size, comparing content...");
  bool isNewer = compareFileContent();
  Serial.printf("[SDCARD] DEBUG: SD is %s based on content\n", isNewer ? "newer" : "older");
  return isNewer;
}

bool SDCardManager::compareFileContent() {
  // Open both files
  File sdFile = SD.open(CONFIG_FILENAME, FILE_READ);
  File spiffsFile = SPIFFS.open(CONFIG_FILENAME, FILE_READ);
  
  if (!sdFile || !spiffsFile) {
    Serial.println("[SDCARD] DEBUG: Failed to open files for content comparison");
    if (sdFile) sdFile.close();
    if (spiffsFile) spiffsFile.close();
    return false;
  }
  
  // Compare content byte by byte
  uint8_t sdBuffer[512];
  uint8_t spiffsBuffer[512];
  size_t bytesCompared = 0;
  
  while (sdFile.available() && spiffsFile.available()) {
    size_t sdBytes = sdFile.read(sdBuffer, sizeof(sdBuffer));
    size_t spiffsBytes = spiffsFile.read(spiffsBuffer, sizeof(spiffsBuffer));
    
    if (sdBytes != spiffsBytes) {
      Serial.printf("[SDCARD] DEBUG: Different read sizes: SD=%d, SPIFFS=%d\n", sdBytes, spiffsBytes);
      sdFile.close();
      spiffsFile.close();
      return true; // Different sizes, assume SD is newer
    }
    
    if (memcmp(sdBuffer, spiffsBuffer, sdBytes) != 0) {
      Serial.printf("[SDCARD] DEBUG: Content differs at byte %d\n", bytesCompared);
      sdFile.close();
      spiffsFile.close();
      return true; // Different content, assume SD is newer
    }
    
    bytesCompared += sdBytes;
  }
  
  // Check if one file has more data
  bool sdHasMore = sdFile.available() > 0;
  bool spiffsHasMore = spiffsFile.available() > 0;
  
  Serial.printf("[SDCARD] DEBUG: Compared %d bytes, SD has more: %s, SPIFFS has more: %s\n", 
                bytesCompared, sdHasMore ? "yes" : "no", spiffsHasMore ? "yes" : "no");
  
  sdFile.close();
  spiffsFile.close();
  
  // If SD has more data, it's newer
  bool isNewer = sdHasMore && !spiffsHasMore;
  if (isNewer) {
    Serial.println("[SDCARD] DEBUG: SD has more data, considering newer");
  } else if (!sdHasMore && !spiffsHasMore) {
    Serial.println("[SDCARD] DEBUG: Files are identical");
  } else {
    Serial.println("[SDCARD] DEBUG: SPIFFS has more data, considering older");
  }
  
  return isNewer;
}

time_t SDCardManager::getFileModTime(File& file) {
  // Try to get actual modification time first
  if (file.getLastWrite()) {
    return file.getLastWrite();
  }
  
  // Fallback: use file size + current time as a rough indicator
  // This is not perfect but better than just file size
  return file.size() + millis() / 1000;
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
