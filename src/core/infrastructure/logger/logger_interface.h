#pragma once
#include <Arduino.h>

/**
 * Logger Interface - Breaks circular dependency with ConfigLoader
 * 
 * This interface provides a clean separation between logging functionality
 * and configuration management, preventing circular dependencies.
 * 
 * The interface uses function pointers to allow dynamic configuration
 * without compile-time dependencies on ConfigLoader.
 */

// Forward declarations for function pointer types
typedef bool (*LogLevelChecker)();
typedef bool (*SilentModeChecker)();

class LoggerInterface {
private:
  static LogLevelChecker debugChecker;
  static LogLevelChecker touchChecker;
  static LogLevelChecker telegramChecker;
  static LogLevelChecker allLogsChecker;
  static SilentModeChecker silentModeChecker;
  
  static bool initialized;
  
public:
  // Initialize the logger interface with configuration checkers
  static void initialize(
    LogLevelChecker debug,
    LogLevelChecker touch,
    LogLevelChecker telegram,
    LogLevelChecker allLogs,
    SilentModeChecker silent
  );
  
  // Check if logging is enabled for specific levels
  static bool isDebugEnabled();
  static bool isTouchEnabled();
  static bool isTelegramEnabled();
  static bool isAllLogsEnabled();
  static bool isSilentMode();
  
  // Check if any logging is enabled (for basic Serial_* macros)
  static bool isAnyLoggingEnabled();
  
  // Reset to default state (useful for testing)
  static void reset();
};
