#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "core/domain/status/status.h"
#include <Arduino.h>

// Forward declarations
class NetworkMonitor;
class DisplayManager;

class TaskManager {
private:
  // Task handles
  static TaskHandle_t display_task_handle;
  static TaskHandle_t scanner_task_handle;
  
  // Event queue
  static QueueHandle_t event_queue;
  
  // Dependencies (injected)
  static NetworkMonitor* networkMonitor;
  static DisplayManager* displayManager;
  
  // Task functions
  static void displayTask(void* pv);
  static void scannerTask(void* pv);
  
  // State
  static bool initialized;
  
public:
  // Initialization
  static bool initialize();
  static void cleanup();
  
  // Task management
  static bool startTasks();
  static void stopTasks();
  
  // Dependency injection
  static void setDependencies(NetworkMonitor* nm, DisplayManager* dm);
  
  // Event queue
  static QueueHandle_t getEventQueue() { return event_queue; }
  static bool sendEvent(const ScanEvent& event);
  static bool receiveEvent(ScanEvent& event, uint32_t timeout_ms = 0);
  
  // Status
  static bool isInitialized() { return initialized; }
  
private:
  // Internal methods
  static void createEventQueue();
  static void createTasks();
};
