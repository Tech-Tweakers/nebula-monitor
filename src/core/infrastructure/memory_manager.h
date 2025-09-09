#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @brief Memory Manager - Manual Garbage Collection for ESP32
 * 
 * This class provides manual memory management and garbage collection
 * for the ESP32 system, which doesn't have automatic GC.
 * It helps prevent memory leaks and system reboots.
 */
class MemoryManager {
public:
  // Singleton pattern
  static MemoryManager& getInstance();
  
  // Memory monitoring
  struct MemoryStats {
    uint32_t freeHeap;
    uint32_t minFreeHeap;
    uint32_t maxAllocatedHeap;
    uint32_t freeStack;
    uint32_t minFreeStack;
    bool lowMemory;
    bool criticalMemory;
  };
  
  // Initialize memory manager
  bool initialize();
  
  // Memory monitoring
  MemoryStats getMemoryStats();
  void printMemoryStats();
  bool isMemoryLow();
  bool isMemoryCritical();
  
  // Garbage collection
  void forceGarbageCollection();
  void forceGarbageCollectionSafe();
  void cleanupStrings();
  void cleanupWiFiClients();
  void cleanupHTTPClients();
  
  // Memory allocation helpers
  String* createString(const String& value);
  void destroyString(String* str);
  
  // Watchdog management
  void feedWatchdog();
  void enableWatchdogFeeding();
  void disableWatchdogFeeding();
  
  // Memory pressure management
  void handleMemoryPressure();
  void emergencyCleanup();
  
  // Task monitoring
  void monitorTaskMemory();
  
private:
  MemoryManager() = default;
  ~MemoryManager() = default;
  MemoryManager(const MemoryManager&) = delete;
  MemoryManager& operator=(const MemoryManager&) = delete;
  
  // Internal state
  bool initialized;
  bool watchdogFeedingEnabled;
  uint32_t lastGCCleanup;
  uint32_t lastMemoryCheck;
  uint32_t lastWatchdogFeed;
  
  // Memory thresholds
  static const uint32_t LOW_MEMORY_THRESHOLD = 50000;    // 50KB
  static const uint32_t CRITICAL_MEMORY_THRESHOLD = 20000; // 20KB
  static const uint32_t GC_INTERVAL_MS = 120000;         // 2 minutes
  static const uint32_t MEMORY_CHECK_INTERVAL_MS = 5000; // 5 seconds
  static const uint32_t WATCHDOG_FEED_INTERVAL_MS = 1000; // 1 second
  
  // String pool for efficient memory management
  struct StringPool {
    String* strings[10];  // Reduced from 20 to 10
    bool inUse[10];
    uint8_t nextIndex;
  } stringPool;
  
  // Internal methods
  void initializeStringPool();
  void cleanupStringPool();
  String* allocateString();
  void deallocateString(String* str);
  
  // Memory monitoring tasks
  static void memoryMonitorTask(void* parameter);
  TaskHandle_t memoryMonitorTaskHandle;
};

/**
 * @brief RAII wrapper for automatic string cleanup
 */
class ManagedString {
public:
  ManagedString(const String& value);
  ~ManagedString();
  
  String& get();
  const String& get() const;
  String* operator->();
  const String* operator->() const;
  
private:
  String* str;
  bool managed;
};

/**
 * @brief Memory pressure handler
 */
class MemoryPressureHandler {
public:
  static void handleLowMemory();
  static void handleCriticalMemory();
  static void emergencyCleanup();
};

#endif // MEMORY_MANAGER_H
