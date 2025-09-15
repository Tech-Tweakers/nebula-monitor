#include <Arduino.h>

// Simulate configuration flags for testing
bool DEBUG_LOGS_ENABLED = true;
bool TOUCH_LOGS_ENABLED = true;
bool TELEGRAM_LOGS_ENABLED = true;
bool ALL_LOGS_ENABLED = true;

// Conditional logging macros (simplified version)
#define LOG_DEBUG(msg) if (DEBUG_LOGS_ENABLED || ALL_LOGS_ENABLED) { Serial.print("[DEBUG] "); Serial.println(msg); }
#define LOG_DEBUG_F(format, ...) if (DEBUG_LOGS_ENABLED || ALL_LOGS_ENABLED) { Serial.printf("[DEBUG] " format "\n", ##__VA_ARGS__); }

#define LOG_TOUCH(msg) if (TOUCH_LOGS_ENABLED || ALL_LOGS_ENABLED) { Serial.print("[TOUCH] "); Serial.println(msg); }
#define LOG_TOUCH_F(format, ...) if (TOUCH_LOGS_ENABLED || ALL_LOGS_ENABLED) { Serial.printf("[TOUCH] " format "\n", ##__VA_ARGS__); }

#define LOG_TELEGRAM(msg) if (TELEGRAM_LOGS_ENABLED || DEBUG_LOGS_ENABLED || ALL_LOGS_ENABLED) { Serial.print("[TELEGRAM] "); Serial.println(msg); }
#define LOG_TELEGRAM_F(format, ...) if (TELEGRAM_LOGS_ENABLED || DEBUG_LOGS_ENABLED || ALL_LOGS_ENABLED) { Serial.printf("[TELEGRAM] " format "\n", ##__VA_ARGS__); }

#define LOG_INFO(msg) if (ALL_LOGS_ENABLED) { Serial.print("[INFO] "); Serial.println(msg); }
#define LOG_INFO_F(format, ...) if (ALL_LOGS_ENABLED) { Serial.printf("[INFO] " format "\n", ##__VA_ARGS__); }

#define LOG_WARN(msg) if (ALL_LOGS_ENABLED) { Serial.print("[WARN] "); Serial.println(msg); }
#define LOG_WARN_F(format, ...) if (ALL_LOGS_ENABLED) { Serial.printf("[WARN] " format "\n", ##__VA_ARGS__); }

#define LOG_ERROR(msg) { Serial.print("[ERROR] "); Serial.println(msg); }
#define LOG_ERROR_F(format, ...) { Serial.printf("[ERROR] " format "\n", ##__VA_ARGS__); }

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("========================================");
  Serial.println("    DEBUG FLAGS TEST");
  Serial.println("========================================");
  
  Serial.println("\n=== Testing Debug Flags (ALL ENABLED) ===");
  
  // Test debug logs
  Serial.println("\n--- Testing DEBUG_LOGS_ENABLED ---");
  LOG_DEBUG("This is a debug message");
  LOG_DEBUG_F("Debug message with format: %d", 123);
  
  // Test touch logs
  Serial.println("\n--- Testing TOUCH_LOGS_ENABLED ---");
  LOG_TOUCH("This is a touch message");
  LOG_TOUCH_F("Touch message with format: %s", "test");
  
  // Test telegram logs
  Serial.println("\n--- Testing TELEGRAM_LOGS_ENABLED ---");
  LOG_TELEGRAM("This is a telegram message");
  LOG_TELEGRAM_F("Telegram message with format: %s", "bot");
  
  // Test info logs
  Serial.println("\n--- Testing ALL_LOGS_ENABLED (INFO) ---");
  LOG_INFO("This is an info message");
  LOG_INFO_F("Info message with format: %f", 3.14);
  
  // Test warn logs
  Serial.println("\n--- Testing ALL_LOGS_ENABLED (WARN) ---");
  LOG_WARN("This is a warning message");
  LOG_WARN_F("Warning message with format: %x", 0xFF);
  
  // Test error logs (always enabled)
  Serial.println("\n--- Testing ERROR logs (always enabled) ---");
  LOG_ERROR("This is an error message");
  LOG_ERROR_F("Error message with format: %s", "critical");
  
  // Now test with flags disabled
  Serial.println("\n=== Testing Debug Flags (ALL DISABLED) ===");
  DEBUG_LOGS_ENABLED = false;
  TOUCH_LOGS_ENABLED = false;
  ALL_LOGS_ENABLED = false;
  
  Serial.println("\n--- Testing with flags disabled ---");
  LOG_DEBUG("This debug message should NOT appear");
  LOG_TOUCH("This touch message should NOT appear");
  LOG_TELEGRAM("This telegram message should NOT appear");
  LOG_INFO("This info message should NOT appear");
  LOG_WARN("This warning message should NOT appear");
  LOG_ERROR("This error message should ALWAYS appear");
  
  // Test individual flags
  Serial.println("\n=== Testing Individual Flags ===");
  
  // Test only DEBUG enabled
  DEBUG_LOGS_ENABLED = true;
  TOUCH_LOGS_ENABLED = false;
  TELEGRAM_LOGS_ENABLED = false;
  ALL_LOGS_ENABLED = false;
  Serial.println("\n--- Only DEBUG enabled ---");
  LOG_DEBUG("This debug message should appear");
  LOG_TOUCH("This touch message should NOT appear");
  LOG_TELEGRAM("This telegram message should appear (DEBUG enables it)");
  LOG_INFO("This info message should NOT appear");
  
  // Test only TOUCH enabled
  DEBUG_LOGS_ENABLED = false;
  TOUCH_LOGS_ENABLED = true;
  TELEGRAM_LOGS_ENABLED = false;
  ALL_LOGS_ENABLED = false;
  Serial.println("\n--- Only TOUCH enabled ---");
  LOG_DEBUG("This debug message should NOT appear");
  LOG_TOUCH("This touch message should appear");
  LOG_TELEGRAM("This telegram message should NOT appear");
  LOG_INFO("This info message should NOT appear");
  
  // Test only TELEGRAM enabled
  DEBUG_LOGS_ENABLED = false;
  TOUCH_LOGS_ENABLED = false;
  TELEGRAM_LOGS_ENABLED = true;
  ALL_LOGS_ENABLED = false;
  Serial.println("\n--- Only TELEGRAM enabled ---");
  LOG_DEBUG("This debug message should NOT appear");
  LOG_TOUCH("This touch message should NOT appear");
  LOG_TELEGRAM("This telegram message should appear");
  LOG_INFO("This info message should NOT appear");
  
  // Test only ALL enabled
  DEBUG_LOGS_ENABLED = false;
  TOUCH_LOGS_ENABLED = false;
  TELEGRAM_LOGS_ENABLED = false;
  ALL_LOGS_ENABLED = true;
  Serial.println("\n--- Only ALL enabled ---");
  LOG_DEBUG("This debug message should appear");
  LOG_TOUCH("This touch message should appear");
  LOG_TELEGRAM("This telegram message should appear");
  LOG_INFO("This info message should appear");
  
  Serial.println("\n========================================");
  Serial.println("    TEST COMPLETED");
  Serial.println("========================================");
}

void loop() {
  delay(1000);
}
