#include "ssl_mutex_manager.h"
#include <Arduino.h>
#include "core/infrastructure/logger/logger.h"

// Static member definitions
SemaphoreHandle_t SSLMutexManager::ssl_mutex = nullptr;
bool SSLMutexManager::initialized = false;
uint32_t SSLMutexManager::lock_count = 0;
uint32_t SSLMutexManager::max_wait_time_ms = 5000;
uint32_t SSLMutexManager::total_locks = 0;
uint32_t SSLMutexManager::total_wait_time_ms = 0;
uint32_t SSLMutexManager::max_wait_time_ever_ms = 0;

bool SSLMutexManager::initialize() {
  if (initialized) {
    Serial_println("[SSL_MUTEX] Already initialized");
    return true;
  }
  
  Serial_println("[SSL_MUTEX] Initializing SSL mutex manager...");
  
  // Create mutex
  ssl_mutex = xSemaphoreCreateMutex();
  if (!ssl_mutex) {
    Serial_println("[SSL_MUTEX] ERROR: Failed to create SSL mutex!");
    return false;
  }
  
  // Reset statistics
  resetStatistics();
  
  initialized = true;
  Serial_println("[SSL_MUTEX] SSL mutex manager initialized successfully!");
  Serial_printf("[SSL_MUTEX] Max wait time: %d ms\n", max_wait_time_ms);
  
  return true;
}

void SSLMutexManager::cleanup() {
  if (!initialized) return;
  
  Serial_println("[SSL_MUTEX] Cleaning up SSL mutex manager...");
  
  // Wait for any pending locks to be released
  uint32_t wait_count = 0;
  while (lock_count > 0 && wait_count < 100) {
    vTaskDelay(pdMS_TO_TICKS(10));
    wait_count++;
  }
  
  if (lock_count > 0) {
    Serial_printf("[SSL_MUTEX] WARNING: %d locks still active during cleanup!\n", lock_count);
  }
  
  // Delete mutex
  if (ssl_mutex) {
    vSemaphoreDelete(ssl_mutex);
    ssl_mutex = nullptr;
  }
  
  initialized = false;
  lock_count = 0;
  
  Serial_println("[SSL_MUTEX] SSL mutex manager cleaned up");
}

bool SSLMutexManager::lockSSL(uint32_t timeout_ms) {
  if (!initialized) {
    Serial_println("[SSL_MUTEX] ERROR: Not initialized!");
    return false;
  }
  
  if (timeout_ms > max_wait_time_ms) {
    timeout_ms = max_wait_time_ms;
  }
  
  return acquireLock(timeout_ms);
}

void SSLMutexManager::unlockSSL() {
  if (!initialized) {
    Serial_println("[SSL_MUTEX] ERROR: Not initialized!");
    return;
  }
  
  releaseLock();
}

bool SSLMutexManager::tryLockSSL() {
  if (!initialized) {
    Serial_println("[SSL_MUTEX] ERROR: Not initialized!");
    return false;
  }
  
  return acquireLock(0); // No timeout = try once
}

bool SSLMutexManager::isLocked() {
  if (!initialized) return false;
  
  return lock_count > 0;
}

void SSLMutexManager::getStatistics(uint32_t& total_locks_out, 
                                   uint32_t& avg_wait_time_ms_out, 
                                   uint32_t& max_wait_time_ms_out) {
  total_locks_out = total_locks;
  max_wait_time_ms_out = max_wait_time_ever_ms;
  
  if (total_locks > 0) {
    avg_wait_time_ms_out = total_wait_time_ms / total_locks;
  } else {
    avg_wait_time_ms_out = 0;
  }
}

void SSLMutexManager::resetStatistics() {
  total_locks = 0;
  total_wait_time_ms = 0;
  max_wait_time_ever_ms = 0;
  lock_count = 0;
}

void SSLMutexManager::setMaxWaitTime(uint32_t max_wait_ms) {
  max_wait_time_ms = max_wait_ms;
  Serial_printf("[SSL_MUTEX] Max wait time set to %d ms\n", max_wait_ms);
}

bool SSLMutexManager::acquireLock(uint32_t timeout_ms) {
  if (!ssl_mutex) return false;
  
  uint32_t start_time = millis();
  TickType_t timeout_ticks = timeout_ms > 0 ? pdMS_TO_TICKS(timeout_ms) : 0;
  
  BaseType_t result = xSemaphoreTake(ssl_mutex, timeout_ticks);
  
  if (result == pdTRUE) {
    uint32_t wait_time = millis() - start_time;
    
    lock_count++;
    total_locks++;
    total_wait_time_ms += wait_time;
    
    if (wait_time > max_wait_time_ever_ms) {
      max_wait_time_ever_ms = wait_time;
    }
    
    Serial_printf("[SSL_MUTEX] Lock acquired (wait: %d ms, count: %d)\n", 
                  wait_time, lock_count);
    
    return true;
  } else {
    uint32_t wait_time = millis() - start_time;
    Serial_printf("[SSL_MUTEX] Lock timeout after %d ms\n", wait_time);
    return false;
  }
}

void SSLMutexManager::releaseLock() {
  if (!ssl_mutex) return;
  
  if (lock_count > 0) {
    lock_count--;
    xSemaphoreGive(ssl_mutex);
    Serial_printf("[SSL_MUTEX] Lock released (count: %d)\n", lock_count);
  } else {
    Serial_println("[SSL_MUTEX] WARNING: Attempted to release lock when count is 0!");
  }
}

// SSLLock RAII wrapper implementation
SSLLock::SSLLock(uint32_t timeout_ms) : locked(false), start_time(millis()) {
  locked = SSLMutexManager::lockSSL(timeout_ms);
  
  if (locked) {
    Serial_printf("[SSL_LOCK] Auto-locked SSL (timeout: %d ms)\n", timeout_ms);
  } else {
    Serial_printf("[SSL_LOCK] Failed to lock SSL within %d ms\n", timeout_ms);
  }
}

SSLLock::~SSLLock() {
  if (locked) {
    SSLMutexManager::unlockSSL();
    uint32_t hold_time = millis() - start_time;
    Serial_printf("[SSL_LOCK] Auto-unlocked SSL (held for %d ms)\n", hold_time);
  }
}

void SSLLock::release() {
  if (locked) {
    SSLMutexManager::unlockSSL();
    locked = false;
    uint32_t hold_time = millis() - start_time;
    Serial_printf("[SSL_LOCK] Manually released SSL (held for %d ms)\n", hold_time);
  }
}

