#include "sdcard_manager.h"
#include "core/infrastructure/logger/logger.h"
#include "config/config_loader/config_loader.h"

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
  
  Serial_println("[SDCARD] Initializing SDCard manager...");
  
  // Get SD CS pin from build flags
  #ifdef SD_CS
    sdCsPin = SD_CS;
  #else
    sdCsPin = DEFAULT_SD_CS_PIN;
  #endif
  
  Serial_printf("[SDCARD] Using SD CS pin: %d\n", sdCsPin);
  
  // Initialize SD Card
  if (!SD.begin(sdCsPin)) {
    Serial_println("[SDCARD] WARNING: SD Card not available");
    Serial_println("[SDCARD] System will use SPIFFS only");
    return false;
  }
  
  Serial_println("[SDCARD] SD Card initialized successfully!");
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
    Serial_println("[SDCARD] SD Card not available for sync");
    return false;
  }
  
  // Check if config exists on SD
  if (!SD.exists(CONFIG_FILENAME)) {
    Serial_println("[SDCARD] No config.env found on SD Card");
    return false;
  }
  
  // Check if SD config is newer
  if (!isSDConfigNewer()) {
    Serial_println("[SDCARD] SD config is not newer, skipping sync");
    return false;
  }
  
  Serial_println("[SDCARD] SD config is newer, syncing to SPIFFS...");
  
  // Open SD file
  File sdFile = SD.open(CONFIG_FILENAME, FILE_READ);
  if (!sdFile) {
    Serial_println("[SDCARD] ERROR: Failed to open SD config file");
    return false;
  }
  
  // Open SPIFFS file for writing
  File spiffsFile = SPIFFS.open(CONFIG_FILENAME, FILE_WRITE);
  if (!spiffsFile) {
    Serial_println("[SDCARD] ERROR: Failed to open SPIFFS config file for writing");
    sdFile.close();
    return false;
  }
  
  // Copy file content
  bool success = copyFile(sdFile, spiffsFile);
  
  // Close files
  sdFile.close();
  spiffsFile.close();
  
  if (success) {
    Serial_println("[SDCARD] SUCCESS: Config synced from SD to SPIFFS!");
    Serial_println("[SDCARD] New configuration loaded successfully!");
    Serial_println("[SDCARD] No restart needed - config will be loaded on next scan cycle");
  } else {
    Serial_println("[SDCARD] ERROR: Failed to sync config from SD");
  }
  
  return success;
}

bool SDCardManager::syncConfigToSD() {
  if (!isSDCardAvailable()) {
    Serial_println("[SDCARD] SD Card not available for sync");
    return false;
  }
  
  // Check if config exists on SPIFFS
  if (!SPIFFS.exists(CONFIG_FILENAME)) {
    Serial_println("[SDCARD] No config.env found on SPIFFS");
    return false;
  }
  
  Serial_println("[SDCARD] Syncing config from SPIFFS to SD...");
  
  // Open SPIFFS file
  File spiffsFile = SPIFFS.open(CONFIG_FILENAME, FILE_READ);
  if (!spiffsFile) {
    Serial_println("[SDCARD] ERROR: Failed to open SPIFFS config file");
    return false;
  }
  
  // Open SD file for writing
  File sdFile = SD.open(CONFIG_FILENAME, FILE_WRITE);
  if (!sdFile) {
    Serial_println("[SDCARD] ERROR: Failed to open SD config file for writing");
    spiffsFile.close();
    return false;
  }
  
  // Copy file content
  bool success = copyFile(spiffsFile, sdFile);
  
  // Close files
  spiffsFile.close();
  sdFile.close();
  
  if (success) {
    Serial_println("[SDCARD] SUCCESS: Config synced from SPIFFS to SD!");
  } else {
    Serial_println("[SDCARD] ERROR: Failed to sync config to SD");
  }
  
  return success;
}

bool SDCardManager::isSDConfigNewer() {
  if (!isSDCardAvailable() || !SD.exists(CONFIG_FILENAME)) {
    Serial_println("[SDCARD] DEBUG: SD not available or config not found");
    return false;
  }
  
  // Check if force sync is enabled
  if (isForceSyncEnabled()) {
    Serial_println("[SDCARD] DEBUG: Force sync enabled, treating SD as newer");
    return true;
  }
  
  // Open SD file
  File sdFile = SD.open(CONFIG_FILENAME, FILE_READ);
  if (!sdFile) {
    Serial_println("[SDCARD] DEBUG: Failed to open SD file");
    return false;
  }
  
  // Open SPIFFS file
  File spiffsFile = SPIFFS.open(CONFIG_FILENAME, FILE_READ);
  if (!spiffsFile) {
    Serial_println("[SDCARD] DEBUG: SPIFFS file not found, SD is newer");
    sdFile.close();
    return true; // SD exists but SPIFFS doesn't
  }
  
  sdFile.close();
  spiffsFile.close();
  
  // Strategy 1: Try timestamp comparison (most reliable when NTP is working)
  bool timestampResult = compareByTimestamp();
  if (timestampResult != false) { // If we got a definitive result
    Serial_println("[SDCARD] DEBUG: Using timestamp comparison result");
    return timestampResult;
  }
  
  // Strategy 2: Compare content hashes (more reliable than size)
  Serial_println("[SDCARD] DEBUG: Timestamps unreliable, comparing content hashes...");
  
  sdFile = SD.open(CONFIG_FILENAME, FILE_READ);
  spiffsFile = SPIFFS.open(CONFIG_FILENAME, FILE_READ);
  
  if (sdFile && spiffsFile) {
    uint32_t sdHash = calculateFileHash(sdFile);
    uint32_t spiffsHash = calculateFileHash(spiffsFile);
    
    Serial_printf("[SDCARD] DEBUG: Hash comparison - SD=0x%08X, SPIFFS=0x%08X\n", sdHash, spiffsHash);
    
    sdFile.close();
    spiffsFile.close();
    
    if (sdHash != spiffsHash) {
      Serial_println("[SDCARD] DEBUG: Content differs, SD is newer");
      return true;
    }
    
    Serial_println("[SDCARD] DEBUG: Content identical, no sync needed");
    return false;
  }
  
  // Strategy 3: Fallback to size comparison (but treat ANY difference as "newer")
  Serial_println("[SDCARD] DEBUG: Hash comparison failed, using size fallback...");
  
  sdFile = SD.open(CONFIG_FILENAME, FILE_READ);
  spiffsFile = SPIFFS.open(CONFIG_FILENAME, FILE_READ);
  
  if (sdFile && spiffsFile) {
    size_t sdSize = sdFile.size();
    size_t spiffsSize = spiffsFile.size();
    
    Serial_printf("[SDCARD] DEBUG: File sizes - SD=%d, SPIFFS=%d\n", sdSize, spiffsSize);
    
    sdFile.close();
    spiffsFile.close();
    
    // NEW LOGIC: Any size difference means SD is newer (fixes the original bug)
    if (sdSize != spiffsSize) {
      Serial_printf("[SDCARD] Size difference detected: SD=%d, SPIFFS=%d\n", sdSize, spiffsSize);
      Serial_println("[SDCARD] DEBUG: SD is newer (any size difference counts)");
      return true;
    }
  }
  
  Serial_println("[SDCARD] DEBUG: All comparisons suggest files are identical");
  return false;
}

bool SDCardManager::compareFileContent() {
  // Open both files
  File sdFile = SD.open(CONFIG_FILENAME, FILE_READ);
  File spiffsFile = SPIFFS.open(CONFIG_FILENAME, FILE_READ);
  
  if (!sdFile || !spiffsFile) {
    Serial_println("[SDCARD] DEBUG: Failed to open files for content comparison");
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
      Serial_printf("[SDCARD] DEBUG: Different read sizes: SD=%d, SPIFFS=%d\n", sdBytes, spiffsBytes);
      sdFile.close();
      spiffsFile.close();
      return true; // Different sizes, assume SD is newer
    }
    
    if (memcmp(sdBuffer, spiffsBuffer, sdBytes) != 0) {
      Serial_printf("[SDCARD] DEBUG: Content differs at byte %d\n", bytesCompared);
      sdFile.close();
      spiffsFile.close();
      return true; // Different content, assume SD is newer
    }
    
    bytesCompared += sdBytes;
  }
  
  // Check if one file has more data
  bool sdHasMore = sdFile.available() > 0;
  bool spiffsHasMore = spiffsFile.available() > 0;
  
  Serial_printf("[SDCARD] DEBUG: Compared %d bytes, SD has more: %s, SPIFFS has more: %s\n", 
                bytesCompared, sdHasMore ? "yes" : "no", spiffsHasMore ? "yes" : "no");
  
  sdFile.close();
  spiffsFile.close();
  
  // If SD has more data, it's newer
  bool isNewer = sdHasMore && !spiffsHasMore;
  if (isNewer) {
    Serial_println("[SDCARD] DEBUG: SD has more data, considering newer");
  } else if (!sdHasMore && !spiffsHasMore) {
    Serial_println("[SDCARD] DEBUG: Files are identical");
  } else {
    Serial_println("[SDCARD] DEBUG: SPIFFS has more data, considering older");
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
      Serial_println("[SDCARD] ERROR: Write mismatch during copy");
      return false;
    }
    
    bytesRead += bytesToRead;
    bytesWritten += bytesToWrite;
  }
  
  Serial_printf("[SDCARD] Copied %d bytes from source to destination\n", bytesWritten);
  return bytesWritten > 0;
}

uint32_t SDCardManager::calculateFileHash(File& file) {
  if (!file) return 0;
  
  // Simple hash function (CRC32-like)
  uint32_t hash = 0;
  uint8_t buffer[64];
  file.seek(0); // Reset to beginning
  
  while (file.available()) {
    size_t bytesRead = file.read(buffer, sizeof(buffer));
    for (size_t i = 0; i < bytesRead; i++) {
      hash = hash * 31 + buffer[i]; // Simple polynomial hash
    }
  }
  
  file.seek(0); // Reset to beginning for caller
  return hash;
}

bool SDCardManager::compareByTimestamp() {
  // Open both files
  File sdFile = SD.open(CONFIG_FILENAME, FILE_READ);
  File spiffsFile = SPIFFS.open(CONFIG_FILENAME, FILE_READ);
  
  if (!sdFile || !spiffsFile) {
    if (sdFile) sdFile.close();
    if (spiffsFile) spiffsFile.close();
    Serial_println("[SDCARD] DEBUG: Could not open files for timestamp comparison");
    return false; // Indicate we couldn't determine
  }
  
  time_t sdTime = getFileModTime(sdFile);
  time_t spiffsTime = getFileModTime(spiffsFile);
  
  sdFile.close();
  spiffsFile.close();
  
  Serial_printf("[SDCARD] DEBUG: Timestamps - SD=%lu, SPIFFS=%lu\n", sdTime, spiffsTime);
  
  // Check if timestamps are valid (after year 2020)
  const time_t year2020 = 1577836800; // Jan 1, 2020 00:00:00 UTC
  bool sdTimeValid = sdTime > year2020;
  bool spiffsTimeValid = spiffsTime > year2020;
  
  if (!sdTimeValid && !spiffsTimeValid) {
    Serial_println("[SDCARD] DEBUG: Both timestamps invalid, cannot use for comparison");
    return false; // Indicate we couldn't determine
  }
  
  if (!spiffsTimeValid && sdTimeValid) {
    Serial_println("[SDCARD] DEBUG: SPIFFS timestamp invalid, SD is newer");
    return true;
  }
  
  if (!sdTimeValid && spiffsTimeValid) {
    Serial_println("[SDCARD] DEBUG: SD timestamp invalid, SPIFFS is newer");
    return false;
  }
  
  // Both timestamps are valid, compare them
  bool isNewer = sdTime > spiffsTime;
  Serial_printf("[SDCARD] DEBUG: Timestamp comparison: SD is %s\n", isNewer ? "newer" : "older or same");
  return isNewer;
}

bool SDCardManager::isForceSyncEnabled() {
  // Check if ConfigLoader is available and initialized
  if (!ConfigLoader::isInitialized()) {
    return false; // Safe default when config is not loaded yet
  }
  
  return ConfigLoader::isSdForceSyncEnabled();
}

void SDCardManager::cleanup() {
  if (initialized) {
    initialized = false;
    Serial_println("[SDCARD] SDCard manager cleaned up");
  }
}
