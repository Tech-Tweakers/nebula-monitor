#include "logger_interface.h"

// Static member definitions
LogLevelChecker LoggerInterface::debugChecker = nullptr;
LogLevelChecker LoggerInterface::touchChecker = nullptr;
LogLevelChecker LoggerInterface::telegramChecker = nullptr;
LogLevelChecker LoggerInterface::allLogsChecker = nullptr;
SilentModeChecker LoggerInterface::silentModeChecker = nullptr;
bool LoggerInterface::initialized = false;

void LoggerInterface::initialize(
  LogLevelChecker debug,
  LogLevelChecker touch,
  LogLevelChecker telegram,
  LogLevelChecker allLogs,
  SilentModeChecker silent
) {
  debugChecker = debug;
  touchChecker = touch;
  telegramChecker = telegram;
  allLogsChecker = allLogs;
  silentModeChecker = silent;
  initialized = true;
}

bool LoggerInterface::isDebugEnabled() {
  if (!initialized || !debugChecker) return false;
  return debugChecker();
}

bool LoggerInterface::isTouchEnabled() {
  if (!initialized || !touchChecker) return false;
  return touchChecker();
}

bool LoggerInterface::isTelegramEnabled() {
  if (!initialized || !telegramChecker) return false;
  return telegramChecker();
}

bool LoggerInterface::isAllLogsEnabled() {
  if (!initialized || !allLogsChecker) return false;
  return allLogsChecker();
}

bool LoggerInterface::isSilentMode() {
  if (!initialized || !silentModeChecker) return false;
  return silentModeChecker();
}

bool LoggerInterface::isAnyLoggingEnabled() {
  return !isSilentMode();
}

void LoggerInterface::reset() {
  debugChecker = nullptr;
  touchChecker = nullptr;
  telegramChecker = nullptr;
  allLogsChecker = nullptr;
  silentModeChecker = nullptr;
  initialized = false;
}
