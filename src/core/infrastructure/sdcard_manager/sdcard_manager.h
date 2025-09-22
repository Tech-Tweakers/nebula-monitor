#pragma once
#include <Arduino.h>
#include <SD.h>
#include <SPIFFS.h>

/**
 * @brief SDCard Manager - Automatic configuration sync between SD and SPIFFS
 * 
 * This class manages automatic synchronization of config.env between SD Card and SPIFFS.
 * It allows users to edit configuration on PC via SD Card and automatically sync to ESP32.
 */
class SDCardManager {
public:
  // Singleton pattern
  static SDCardManager& getInstance();
  
  // Initialize SDCard manager
  bool initialize();
  
  // Check if SDCard is available
  bool isSDCardAvailable();
  
  // Sync configuration from SD to SPIFFS if SD is newer
  bool syncConfigFromSD();
  
  // Sync configuration from SPIFFS to SD
  bool syncConfigToSD();
  
  // Check if SD config is newer than SPIFFS (improved logic)
  bool isSDConfigNewer();
  
  // Get file modification time
  time_t getFileModTime(File& file);
  
  // Compare file content byte by byte
  bool compareFileContent();
  
  // New: Calculate simple hash of file content
  uint32_t calculateFileHash(File& file);
  
  // New: Compare using timestamps (most reliable when available)
  // Returns: 1 = SD newer, 0 = same/older, -1 = cannot determine
  int compareByTimestamp();
  
  // New: Check if force sync is enabled
  bool isForceSyncEnabled();
  
  // Copy file from source to destination
  bool copyFile(File& source, File& destination);
  
  // Cleanup
  void cleanup();
  
  // Status
  bool isInitialized() const { return initialized; }
  
private:
  SDCardManager() = default;
  ~SDCardManager() = default;
  SDCardManager(const SDCardManager&) = delete;
  SDCardManager& operator=(const SDCardManager&) = delete;
  
  // Internal state
  static bool initialized;
  static int sdCsPin;
  
  // Constants
  static const int DEFAULT_SD_CS_PIN = 5;
  static const char* CONFIG_FILENAME;
};
