#pragma once
#include <Arduino.h>
#include "logger_interface.h"

/**
 * New Logger System - Fixed Circular Dependency and Control Flow Issues
 * 
 * This logger system:
 * 1. Breaks circular dependency with ConfigLoader using LoggerInterface
 * 2. Uses do-while(0) pattern to prevent dangling else problems
 * 3. Provides proper block scoping for all macros
 * 4. Handles initialization order properly
 */

// Silent mode macro - completely disables Serial when SILENT_MODE is enabled
#define SERIAL_IF_NOT_SILENT do { if (LoggerInterface::isAnyLoggingEnabled()) {
#define SERIAL_END_IF } } while(0)

// Direct Serial replacement macros for silent mode (fixed control flow)
#define Serial_print(msg) do { if (LoggerInterface::isAnyLoggingEnabled()) { Serial.print(msg); } } while(0)
#define Serial_println(msg) do { if (LoggerInterface::isAnyLoggingEnabled()) { Serial.println(msg); } } while(0)
#define Serial_printf(format, ...) do { if (LoggerInterface::isAnyLoggingEnabled()) { Serial.printf(format, ##__VA_ARGS__); } } while(0)

// Conditional logging macros (fixed control flow with do-while(0))
#define LOG_DEBUG(msg) do { \
  if (LoggerInterface::isAnyLoggingEnabled() && (LoggerInterface::isDebugEnabled() || LoggerInterface::isAllLogsEnabled())) { \
    Serial.print("[DEBUG] "); \
    Serial.println(msg); \
  } \
} while(0)

#define LOG_DEBUG_F(format, ...) do { \
  if (LoggerInterface::isAnyLoggingEnabled() && (LoggerInterface::isDebugEnabled() || LoggerInterface::isAllLogsEnabled())) { \
    Serial.printf("[DEBUG] " format "\n", ##__VA_ARGS__); \
  } \
} while(0)

#define LOG_TOUCH(msg) do { \
  if (LoggerInterface::isAnyLoggingEnabled() && (LoggerInterface::isTouchEnabled() || LoggerInterface::isAllLogsEnabled())) { \
    Serial.print("[TOUCH] "); \
    Serial.println(msg); \
  } \
} while(0)

#define LOG_TOUCH_F(format, ...) do { \
  if (LoggerInterface::isAnyLoggingEnabled() && (LoggerInterface::isTouchEnabled() || LoggerInterface::isAllLogsEnabled())) { \
    Serial.printf("[TOUCH] " format "\n", ##__VA_ARGS__); \
  } \
} while(0)

#define LOG_INFO(msg) do { \
  if (LoggerInterface::isAnyLoggingEnabled() && LoggerInterface::isAllLogsEnabled()) { \
    Serial.print("[INFO] "); \
    Serial.println(msg); \
  } \
} while(0)

#define LOG_INFO_F(format, ...) do { \
  if (LoggerInterface::isAnyLoggingEnabled() && LoggerInterface::isAllLogsEnabled()) { \
    Serial.printf("[INFO] " format "\n", ##__VA_ARGS__); \
  } \
} while(0)

#define LOG_WARN(msg) do { \
  if (LoggerInterface::isAnyLoggingEnabled() && LoggerInterface::isAllLogsEnabled()) { \
    Serial.print("[WARN] "); \
    Serial.println(msg); \
  } \
} while(0)

#define LOG_WARN_F(format, ...) do { \
  if (LoggerInterface::isAnyLoggingEnabled() && LoggerInterface::isAllLogsEnabled()) { \
    Serial.printf("[WARN] " format "\n", ##__VA_ARGS__); \
  } \
} while(0)

#define LOG_ERROR(msg) do { \
  if (LoggerInterface::isAnyLoggingEnabled()) { \
    Serial.print("[ERROR] "); \
    Serial.println(msg); \
  } \
} while(0)

#define LOG_ERROR_F(format, ...) do { \
  if (LoggerInterface::isAnyLoggingEnabled()) { \
    Serial.printf("[ERROR] " format "\n", ##__VA_ARGS__); \
  } \
} while(0)

// Always enabled logs (critical system messages)
#define LOG_CRITICAL(msg) do { \
  if (LoggerInterface::isAnyLoggingEnabled()) { \
    Serial.print("[CRITICAL] "); \
    Serial.println(msg); \
  } \
} while(0)

#define LOG_CRITICAL_F(format, ...) do { \
  if (LoggerInterface::isAnyLoggingEnabled()) { \
    Serial.printf("[CRITICAL] " format "\n", ##__VA_ARGS__); \
  } \
} while(0)

// Service-specific logging macros
#define LOG_TELEGRAM(msg) do { \
  if (LoggerInterface::isAnyLoggingEnabled() && (LoggerInterface::isTelegramEnabled() || LoggerInterface::isDebugEnabled() || LoggerInterface::isAllLogsEnabled())) { \
    Serial.print("[TELEGRAM] "); \
    Serial.println(msg); \
  } \
} while(0)

#define LOG_TELEGRAM_F(format, ...) do { \
  if (LoggerInterface::isAnyLoggingEnabled() && (LoggerInterface::isTelegramEnabled() || LoggerInterface::isDebugEnabled() || LoggerInterface::isAllLogsEnabled())) { \
    Serial.printf("[TELEGRAM] " format "\n", ##__VA_ARGS__); \
  } \
} while(0)

#define LOG_DISPLAY(msg) do { \
  if (LoggerInterface::isAnyLoggingEnabled() && (LoggerInterface::isDebugEnabled() || LoggerInterface::isAllLogsEnabled())) { \
    Serial.print("[DISPLAY] "); \
    Serial.println(msg); \
  } \
} while(0)

#define LOG_DISPLAY_F(format, ...) do { \
  if (LoggerInterface::isAnyLoggingEnabled() && (LoggerInterface::isDebugEnabled() || LoggerInterface::isAllLogsEnabled())) { \
    Serial.printf("[DISPLAY] " format "\n", ##__VA_ARGS__); \
  } \
} while(0)

#define LOG_HTTP(msg) do { \
  if (LoggerInterface::isAnyLoggingEnabled() && (LoggerInterface::isDebugEnabled() || LoggerInterface::isAllLogsEnabled())) { \
    Serial.print("[HTTP] "); \
    Serial.println(msg); \
  } \
} while(0)

#define LOG_HTTP_F(format, ...) do { \
  if (LoggerInterface::isAnyLoggingEnabled() && (LoggerInterface::isDebugEnabled() || LoggerInterface::isAllLogsEnabled())) { \
    Serial.printf("[HTTP] " format "\n", ##__VA_ARGS__); \
  } \
} while(0)

#define LOG_MEMORY(msg) do { \
  if (LoggerInterface::isAnyLoggingEnabled() && (LoggerInterface::isDebugEnabled() || LoggerInterface::isAllLogsEnabled())) { \
    Serial.print("[MEMORY] "); \
    Serial.println(msg); \
  } \
} while(0)

#define LOG_MEMORY_F(format, ...) do { \
  if (LoggerInterface::isAnyLoggingEnabled() && (LoggerInterface::isDebugEnabled() || LoggerInterface::isAllLogsEnabled())) { \
    Serial.printf("[MEMORY] " format "\n", ##__VA_ARGS__); \
  } \
} while(0)

#define LOG_NETWORK(msg) do { \
  if (LoggerInterface::isAnyLoggingEnabled() && (LoggerInterface::isDebugEnabled() || LoggerInterface::isAllLogsEnabled())) { \
    Serial.print("[NETWORK] "); \
    Serial.println(msg); \
  } \
} while(0)

#define LOG_NETWORK_F(format, ...) do { \
  if (LoggerInterface::isAnyLoggingEnabled() && (LoggerInterface::isDebugEnabled() || LoggerInterface::isAllLogsEnabled())) { \
    Serial.printf("[NETWORK] " format "\n", ##__VA_ARGS__); \
  } \
} while(0)

#define LOG_CONFIG(msg) do { \
  if (LoggerInterface::isAnyLoggingEnabled() && (LoggerInterface::isDebugEnabled() || LoggerInterface::isAllLogsEnabled())) { \
    Serial.print("[CONFIG] "); \
    Serial.println(msg); \
  } \
} while(0)

#define LOG_CONFIG_F(format, ...) do { \
  if (LoggerInterface::isAnyLoggingEnabled() && (LoggerInterface::isDebugEnabled() || LoggerInterface::isAllLogsEnabled())) { \
    Serial.printf("[CONFIG] " format "\n", ##__VA_ARGS__); \
  } \
} while(0)

#define LOG_MAIN(msg) do { \
  if (LoggerInterface::isAnyLoggingEnabled() && (LoggerInterface::isDebugEnabled() || LoggerInterface::isAllLogsEnabled())) { \
    Serial.print("[MAIN] "); \
    Serial.println(msg); \
  } \
} while(0)

#define LOG_MAIN_F(format, ...) do { \
  if (LoggerInterface::isAnyLoggingEnabled() && (LoggerInterface::isDebugEnabled() || LoggerInterface::isAllLogsEnabled())) { \
    Serial.printf("[MAIN] " format "\n", ##__VA_ARGS__); \
  } \
} while(0)

// Legacy support - these will always print (for backward compatibility during transition)
#define LOG_LEGACY(msg) do { \
  if (LoggerInterface::isAnyLoggingEnabled()) { \
    Serial.println(msg); \
  } \
} while(0)

#define LOG_LEGACY_F(format, ...) do { \
  if (LoggerInterface::isAnyLoggingEnabled()) { \
    Serial.printf(format "\n", ##__VA_ARGS__); \
  } \
} while(0)
