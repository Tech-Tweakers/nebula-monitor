#include "core/infrastructure/task_manager/task_manager.h"
#include "core/domain/network_monitor/network_monitor.h"
#include "ui/display_manager.h"
#include "core/infrastructure/memory_manager.h"
#include <Arduino.h>

// Static member definitions
TaskHandle_t TaskManager::display_task_handle = nullptr;
TaskHandle_t TaskManager::scanner_task_handle = nullptr;
QueueHandle_t TaskManager::event_queue = nullptr;
NetworkMonitor* TaskManager::networkMonitor = nullptr;
DisplayManager* TaskManager::displayManager = nullptr;
bool TaskManager::initialized = false;

bool TaskManager::initialize() {
  if (initialized) return true;
  
  Serial.println("[TASK_MANAGER] Initializing task manager...");
  
  // Create event queue
  createEventQueue();
  
  // Create tasks
  createTasks();
  
  initialized = true;
  Serial.println("[TASK_MANAGER] Task manager initialized successfully!");
  
  return true;
}

void TaskManager::cleanup() {
  if (!initialized) return;
  
  // Stop tasks
  stopTasks();
  
  // Delete event queue
  if (event_queue) {
    vQueueDelete(event_queue);
    event_queue = nullptr;
  }
  
  initialized = false;
  Serial.println("[TASK_MANAGER] Task manager cleaned up");
}

bool TaskManager::startTasks() {
  if (!initialized) {
    Serial.println("[TASK_MANAGER] ERROR: Not initialized!");
    return false;
  }
  
  if (display_task_handle || scanner_task_handle) {
    Serial.println("[TASK_MANAGER] Tasks already running!");
    return true;
  }
  
  // Create tasks
  createTasks();
  
  Serial.println("[TASK_MANAGER] Tasks started successfully!");
  return true;
}

void TaskManager::stopTasks() {
  if (display_task_handle) {
    vTaskDelete(display_task_handle);
    display_task_handle = nullptr;
  }
  
  if (scanner_task_handle) {
    vTaskDelete(scanner_task_handle);
    scanner_task_handle = nullptr;
  }
  
  Serial.println("[TASK_MANAGER] Tasks stopped");
}

void TaskManager::setDependencies(NetworkMonitor* nm, DisplayManager* dm) {
  networkMonitor = nm;
  displayManager = dm;
}

bool TaskManager::sendEvent(const ScanEvent& event) {
  if (!event_queue) return false;
  
  return xQueueSend(event_queue, &event, 0) == pdTRUE;
}

bool TaskManager::receiveEvent(ScanEvent& event, uint32_t timeout_ms) {
  if (!event_queue) return false;
  
  TickType_t timeout = timeout_ms > 0 ? pdMS_TO_TICKS(timeout_ms) : 0;
  return xQueueReceive(event_queue, &event, timeout) == pdTRUE;
}

void TaskManager::createEventQueue() {
  event_queue = xQueueCreate(20, sizeof(ScanEvent));
  if (!event_queue) {
    Serial.println("[TASK_MANAGER] ERROR: Failed to create event queue!");
  } else {
    Serial.println("[TASK_MANAGER] Event queue created successfully");
  }
}

void TaskManager::createTasks() {
  // Create display task (Core 1, higher priority)
  BaseType_t result = xTaskCreatePinnedToCore(
    displayTask,
    "DisplayTask",
    4096,        // Stack size: 4KB
    nullptr,
    3,           // Priority: 3 (higher)
    &display_task_handle,
    1            // Core 1
  );
  
  if (result != pdPASS) {
    Serial.println("[TASK_MANAGER] ERROR: Failed to create display task!");
    return;
  }
  
  // Create scanner task (Core 0, lower priority)
  result = xTaskCreatePinnedToCore(
    scannerTask,
    "ScannerTask",
    8192,        // Stack size: 8KB (increased to prevent overflow)
    nullptr,
    2,           // Priority: 2 (lower)
    &scanner_task_handle,
    0            // Core 0
  );
  
  if (result != pdPASS) {
    Serial.println("[TASK_MANAGER] ERROR: Failed to create scanner task!");
    return;
  }
  
  Serial.println("[TASK_MANAGER] Tasks created successfully!");
}

void TaskManager::displayTask(void* pv) {
  Serial.println("[DISPLAY_TASK] Started on Core 1");
  
  for (;;) {
    // Process events from queue
    ScanEvent event;
    while (receiveEvent(event, 0)) { // Non-blocking
      if (displayManager) {
        switch (event.type) {
          case EV_SCAN_START:
            displayManager->onScanStarted();
            break;
          case EV_SCAN_COMPLETE:
            displayManager->onScanCompleted();
            break;
          case EV_TARGET_UPDATE:
            displayManager->updateTargetStatus(event.index, event.status, event.latency_ms);
            break;
        }
      }
    }
    
    // Update display manager
    if (displayManager) {
      displayManager->update();
    }
    
    // Small delay to prevent CPU hogging
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void TaskManager::scannerTask(void* pv) {
  Serial.println("[SCANNER_TASK] Started on Core 0");
  
  UBaseType_t stackHighWaterMark = 0;
  uint32_t lastStackCheck = 0;
  uint32_t lastMemoryCheck = 0;
  
  for (;;) {
    uint32_t now = millis();
    
    // Monitor stack usage every 30 seconds
    if (now - lastStackCheck > 30000) {
      stackHighWaterMark = uxTaskGetStackHighWaterMark(nullptr);
      Serial.printf("[SCANNER_TASK] Stack high water mark: %d bytes\n", stackHighWaterMark * sizeof(StackType_t));
      
      if (stackHighWaterMark < 512) { // Less than 512 bytes remaining
        Serial.println("[SCANNER_TASK] WARNING: Low stack space!");
      }
      
      lastStackCheck = now;
    }
    
    // Check memory every 10 seconds
    if (now - lastMemoryCheck > 10000) {
      // Only run GC if NOT scanning to avoid interrupting active scans
      if (MemoryManager::getInstance().isMemoryLow()) {
        if (networkMonitor && !networkMonitor->isScanning()) {
          Serial.println("[SCANNER_TASK] Low memory detected, triggering GC (scan not active)");
          MemoryManager::getInstance().forceGarbageCollection();
        } else if (networkMonitor && networkMonitor->isScanning()) {
          Serial.println("[SCANNER_TASK] Low memory detected but scan active, deferring GC");
        } else {
          Serial.println("[SCANNER_TASK] Low memory detected, triggering GC");
          MemoryManager::getInstance().forceGarbageCollection();
        }
      }
      lastMemoryCheck = now;
    }
    
    // Feed watchdog
    MemoryManager::getInstance().feedWatchdog();
    
    // Update network monitor
    if (networkMonitor) {
      // Check for stuck scans
      if (networkMonitor->isScanStuck()) {
        Serial.println("[SCANNER_TASK] WARNING: Detected stuck scan, forcing stop");
        networkMonitor->forceStopScan();
      }
      
      networkMonitor->update();
    }
    
    // Adaptive delay based on activity
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}
