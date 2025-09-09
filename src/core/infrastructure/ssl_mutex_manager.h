#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <Arduino.h>

/**
 * @brief SSL Mutex Manager - Prevents SSL memory allocation conflicts
 * 
 * This class manages a global mutex for SSL operations to prevent
 * memory allocation conflicts when multiple tasks try to use SSL/TLS
 * simultaneously. This is crucial for ESP32 systems with limited memory.
 */
class SSLMutexManager {
private:
  static SemaphoreHandle_t ssl_mutex;
  static bool initialized;
  static uint32_t lock_count;
  static uint32_t max_wait_time_ms;
  
  // Statistics
  static uint32_t total_locks;
  static uint32_t total_wait_time_ms;
  static uint32_t max_wait_time_ever_ms;
  
public:
  /**
   * @brief Initialize the SSL mutex manager
   * @return true if successful, false otherwise
   */
  static bool initialize();
  
  /**
   * @brief Cleanup the SSL mutex manager
   */
  static void cleanup();
  
  /**
   * @brief Lock SSL operations (blocking)
   * @param timeout_ms Maximum time to wait for lock (0 = no timeout)
   * @return true if lock acquired, false if timeout
   */
  static bool lockSSL(uint32_t timeout_ms = 5000);
  
  /**
   * @brief Unlock SSL operations
   */
  static void unlockSSL();
  
  /**
   * @brief Try to lock SSL operations (non-blocking)
   * @return true if lock acquired, false if already locked
   */
  static bool tryLockSSL();
  
  /**
   * @brief Check if SSL is currently locked
   * @return true if locked, false if available
   */
  static bool isLocked();
  
  /**
   * @brief Get current lock count (for debugging)
   * @return Current number of locks
   */
  static uint32_t getLockCount() { return lock_count; }
  
  /**
   * @brief Get statistics
   * @param total_locks_out Total number of locks acquired
   * @param avg_wait_time_ms_out Average wait time in milliseconds
   * @param max_wait_time_ms_out Maximum wait time ever recorded
   */
  static void getStatistics(uint32_t& total_locks_out, 
                          uint32_t& avg_wait_time_ms_out, 
                          uint32_t& max_wait_time_ms_out);
  
  /**
   * @brief Reset statistics
   */
  static void resetStatistics();
  
  /**
   * @brief Set maximum wait time for locks
   * @param max_wait_ms Maximum wait time in milliseconds
   */
  static void setMaxWaitTime(uint32_t max_wait_ms);
  
  /**
   * @brief Check if initialized
   * @return true if initialized, false otherwise
   */
  static bool isInitialized() { return initialized; }
  
private:
  /**
   * @brief Internal method to acquire lock with timing
   * @param timeout_ms Timeout in milliseconds
   * @return true if successful, false if timeout
   */
  static bool acquireLock(uint32_t timeout_ms);
  
  /**
   * @brief Internal method to release lock
   */
  static void releaseLock();
};

// RAII wrapper for automatic SSL lock management
class SSLLock {
private:
  bool locked;
  uint32_t start_time;
  
public:
  /**
   * @brief Constructor - automatically locks SSL
   * @param timeout_ms Maximum time to wait for lock
   */
  explicit SSLLock(uint32_t timeout_ms = 5000);
  
  /**
   * @brief Destructor - automatically unlocks SSL
   */
  ~SSLLock();
  
  /**
   * @brief Check if lock was successfully acquired
   * @return true if locked, false if failed
   */
  bool isLocked() const { return locked; }
  
  /**
   * @brief Manually release lock (optional)
   */
  void release();
  
  // Disable copy constructor and assignment
  SSLLock(const SSLLock&) = delete;
  SSLLock& operator=(const SSLLock&) = delete;
};

