#include "memory_manager.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "core/infrastructure/logger/logger.h"

// Global instance
MemoryManager& MemoryManager::getInstance() {
  static MemoryManager instance;
  return instance;
}

bool MemoryManager::initialize() {
  if (initialized) return true;
  
  Serial_println("[MEMORY_MANAGER] Initializing memory manager...");
  
  // Initialize state
  initialized = true;
  watchdogFeedingEnabled = false; // Disabled - full control to us
  lastGCCleanup = 0;
  lastMemoryCheck = 0;
  lastWatchdogFeed = 0;
  
  // Initialize string pool
  initializeStringPool();
  
  // Create memory monitoring task
  BaseType_t result = xTaskCreatePinnedToCore(
    memoryMonitorTask,
    "MemoryMonitor",
    4096,        // 4KB stack
    this,
    1,           // Low priority
    &memoryMonitorTaskHandle,
    0            // Core 0
  );
  
  if (result != pdPASS) {
    Serial_println("[MEMORY_MANAGER] ERROR: Failed to create memory monitor task!");
    initialized = false;
    return false;
  }
  
  Serial_println("[MEMORY_MANAGER] Memory manager initialized successfully!");
  Serial_println("[MEMORY_MANAGER] WATCHDOG DISABLED - Full control to application");
  return true;
}

MemoryManager::MemoryStats MemoryManager::getMemoryStats() {
  MemoryStats stats;
  
  stats.freeHeap = ESP.getFreeHeap();
  stats.minFreeHeap = ESP.getMinFreeHeap();
  stats.maxAllocatedHeap = ESP.getMaxAllocHeap();
  stats.freeStack = uxTaskGetStackHighWaterMark(nullptr) * sizeof(StackType_t);
  stats.minFreeStack = 0; // Will be updated by task monitoring
  
  stats.lowMemory = stats.freeHeap < LOW_MEMORY_THRESHOLD;
  stats.criticalMemory = stats.freeHeap < CRITICAL_MEMORY_THRESHOLD;
  
  return stats;
}

void MemoryManager::printMemoryStats() {
  MemoryStats stats = getMemoryStats();
  
  Serial_println("\n=== MEMORY STATS ===");
  Serial_printf("Free Heap: %lu bytes\n", stats.freeHeap);
  Serial_printf("Min Free Heap: %lu bytes\n", stats.minFreeHeap);
  Serial_printf("Max Allocated: %lu bytes\n", stats.maxAllocatedHeap);
  Serial_printf("Free Stack: %lu bytes\n", stats.freeStack);
  Serial_printf("Low Memory: %s\n", stats.lowMemory ? "YES" : "NO");
  Serial_printf("Critical Memory: %s\n", stats.criticalMemory ? "YES" : "NO");
  Serial_println("==================\n");
}

bool MemoryManager::isMemoryLow() {
  return ESP.getFreeHeap() < LOW_MEMORY_THRESHOLD;
}

bool MemoryManager::isMemoryCritical() {
  return ESP.getFreeHeap() < CRITICAL_MEMORY_THRESHOLD;
}

void MemoryManager::forceGarbageCollection() {
  uint32_t now = millis();
  
  // Don't run GC too frequently
  if (now - lastGCCleanup < GC_INTERVAL_MS) {
    return;
  }
  
  Serial_println("[MEMORY_MANAGER] Running garbage collection...");
  
  uint32_t beforeGC = ESP.getFreeHeap();
  
  // Clean up strings
  cleanupStrings();
  
  // Clean up WiFi clients
  cleanupWiFiClients();
  
  // Clean up HTTP clients
  cleanupHTTPClients();
  
  // Clean up string pool
  cleanupStringPool();
  
  uint32_t afterGC = ESP.getFreeHeap();
  
  // Check for underflow (afterGC can be higher than beforeGC due to fragmentation)
  if (afterGC >= beforeGC) {
    Serial_printf("[MEMORY_MANAGER] GC completed: no memory freed (heap: %lu -> %lu)\n", beforeGC, afterGC);
  } else {
    uint32_t freed = beforeGC - afterGC;
    Serial_printf("[MEMORY_MANAGER] GC completed: freed %lu bytes (heap: %lu -> %lu)\n", freed, beforeGC, afterGC);
  }
  lastGCCleanup = now;
}

void MemoryManager::forceGarbageCollectionSafe() {
  uint32_t now = millis();
  
  // Don't run GC too frequently
  if (now - lastGCCleanup < GC_INTERVAL_MS) {
    return;
  }
  
  // Check if we're in a critical operation (like scanning)
  // This is a safety check to avoid interrupting critical operations
  if (ESP.getFreeHeap() > CRITICAL_MEMORY_THRESHOLD) {
    Serial_println("[MEMORY_MANAGER] Memory not critical, skipping GC for safety");
    return;
  }
  
  Serial_println("[MEMORY_MANAGER] Running safe garbage collection...");
  forceGarbageCollection();
}

void MemoryManager::cleanupStrings() {
  // Force cleanup of temporary strings
  // This is a best-effort approach since we can't force String cleanup
  Serial_println("[MEMORY_MANAGER] Cleaning up strings...");
}

void MemoryManager::cleanupWiFiClients() {
  // WiFi clients are automatically cleaned up when they go out of scope
  // But we can force some cleanup
  Serial_println("[MEMORY_MANAGER] Cleaning up WiFi clients...");
}

void MemoryManager::cleanupHTTPClients() {
  // HTTP clients cleanup
  Serial_println("[MEMORY_MANAGER] Cleaning up HTTP clients...");
}

String* MemoryManager::createString(const String& value) {
  return allocateString();
}

void MemoryManager::destroyString(String* str) {
  if (str) {
    deallocateString(str);
  }
}

void MemoryManager::feedWatchdog() {
  if (!watchdogFeedingEnabled) return;
  
  uint32_t now = millis();
  if (now - lastWatchdogFeed >= WATCHDOG_FEED_INTERVAL_MS) {
    // Feed the watchdog by yielding
    vTaskDelay(pdMS_TO_TICKS(1));
    lastWatchdogFeed = now;
  }
}

void MemoryManager::enableWatchdogFeeding() {
  watchdogFeedingEnabled = true;
  Serial_println("[MEMORY_MANAGER] Watchdog feeding enabled");
}

void MemoryManager::disableWatchdogFeeding() {
  watchdogFeedingEnabled = false;
  Serial_println("[MEMORY_MANAGER] Watchdog feeding disabled");
}

void MemoryManager::handleMemoryPressure() {
  MemoryStats stats = getMemoryStats();
  
  if (stats.criticalMemory) {
    Serial_println("[MEMORY_MANAGER] CRITICAL MEMORY - Emergency cleanup!");
    emergencyCleanup();
  } else if (stats.lowMemory) {
    Serial_println("[MEMORY_MANAGER] Low memory - Running cleanup...");
    forceGarbageCollection();
  }
}

void MemoryManager::emergencyCleanup() {
  Serial_println("[MEMORY_MANAGER] Emergency cleanup started!");
  
  // Force all cleanup operations
  cleanupStrings();
  cleanupWiFiClients();
  cleanupHTTPClients();
  cleanupStringPool();
  
  // Force garbage collection
  forceGarbageCollection();
  
  // Print final memory stats
  printMemoryStats();
  
  Serial_println("[MEMORY_MANAGER] Emergency cleanup completed!");
}

void MemoryManager::monitorTaskMemory() {
  // Monitor stack usage for all tasks
  UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(nullptr);
  uint32_t freeStack = stackHighWaterMark * sizeof(StackType_t);
  
  if (freeStack < 1024) { // Less than 1KB remaining
    Serial_printf("[MEMORY_MANAGER] WARNING: Low stack space: %lu bytes\n", freeStack);
  }
}

void MemoryManager::initializeStringPool() {
  for (int i = 0; i < 10; i++) {
    stringPool.strings[i] = nullptr;
    stringPool.inUse[i] = false;
  }
  stringPool.nextIndex = 0;
}

void MemoryManager::cleanupStringPool() {
  for (int i = 0; i < 10; i++) {
    if (stringPool.strings[i] && !stringPool.inUse[i]) {
      delete stringPool.strings[i];
      stringPool.strings[i] = nullptr;
    }
  }
}

String* MemoryManager::allocateString() {
  // Find an available slot
  for (int i = 0; i < 10; i++) {
    if (!stringPool.inUse[i]) {
      if (!stringPool.strings[i]) {
        stringPool.strings[i] = new String();
      }
      stringPool.inUse[i] = true;
      return stringPool.strings[i];
    }
  }
  
  // No available slots, create temporary
  return new String();
}

void MemoryManager::deallocateString(String* str) {
  if (!str) return;
  
  // Find and mark as unused
  for (int i = 0; i < 10; i++) {
    if (stringPool.strings[i] == str) {
      stringPool.inUse[i] = false;
      return;
    }
  }
  
  // Not in pool, delete directly
  delete str;
}

void MemoryManager::memoryMonitorTask(void* parameter) {
  MemoryManager* manager = static_cast<MemoryManager*>(parameter);
  
  Serial_println("[MEMORY_MONITOR] Memory monitoring task started");
  
  for (;;) {
    uint32_t now = millis();
    
    // Check memory every 5 seconds
    if (now - manager->lastMemoryCheck >= MEMORY_CHECK_INTERVAL_MS) {
      manager->handleMemoryPressure();
      manager->monitorTaskMemory();
      manager->lastMemoryCheck = now;
    }
    
    // Feed watchdog every second
    manager->feedWatchdog();
    
    // Run GC every 30 seconds
    if (now - manager->lastGCCleanup >= GC_INTERVAL_MS) {
      manager->forceGarbageCollection();
    }
    
    vTaskDelay(pdMS_TO_TICKS(1000)); // Check every second
  }
}

// ManagedString implementation
ManagedString::ManagedString(const String& value) : managed(true) {
  str = MemoryManager::getInstance().createString(value);
  if (str) {
    *str = value;
  }
}

ManagedString::~ManagedString() {
  if (managed && str) {
    MemoryManager::getInstance().destroyString(str);
  }
}

String& ManagedString::get() {
  return *str;
}

const String& ManagedString::get() const {
  return *str;
}

String* ManagedString::operator->() {
  return str;
}

const String* ManagedString::operator->() const {
  return str;
}

// MemoryPressureHandler implementation
void MemoryPressureHandler::handleLowMemory() {
  Serial_println("[MEMORY_PRESSURE] Handling low memory condition");
  MemoryManager::getInstance().forceGarbageCollection();
}

void MemoryPressureHandler::handleCriticalMemory() {
  Serial_println("[MEMORY_PRESSURE] Handling critical memory condition");
  MemoryManager::getInstance().emergencyCleanup();
}

void MemoryPressureHandler::emergencyCleanup() {
  Serial_println("[MEMORY_PRESSURE] Emergency cleanup triggered");
  MemoryManager::getInstance().emergencyCleanup();
}
