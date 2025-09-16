#pragma once
#include <Arduino.h>
#include "config/config_loader/config_loader.h"

// Conditional logging macros
#define LOG_DEBUG(msg) if (!ConfigLoader::isSilentMode() && (ConfigLoader::isDebugLogsEnabled() || ConfigLoader::isAllLogsEnabled())) { Serial.print("[DEBUG] "); Serial.println(msg); }
#define LOG_DEBUG_F(format, ...) if (!ConfigLoader::isSilentMode() && (ConfigLoader::isDebugLogsEnabled() || ConfigLoader::isAllLogsEnabled())) { Serial.printf("[DEBUG] " format "\n", ##__VA_ARGS__); }

#define LOG_TOUCH(msg) if (!ConfigLoader::isSilentMode() && (ConfigLoader::isTouchLogsEnabled() || ConfigLoader::isAllLogsEnabled())) { Serial.print("[TOUCH] "); Serial.println(msg); }
#define LOG_TOUCH_F(format, ...) if (!ConfigLoader::isSilentMode() && (ConfigLoader::isTouchLogsEnabled() || ConfigLoader::isAllLogsEnabled())) { Serial.printf("[TOUCH] " format "\n", ##__VA_ARGS__); }

#define LOG_INFO(msg) if (!ConfigLoader::isSilentMode() && ConfigLoader::isAllLogsEnabled()) { Serial.print("[INFO] "); Serial.println(msg); }
#define LOG_INFO_F(format, ...) if (!ConfigLoader::isSilentMode() && ConfigLoader::isAllLogsEnabled()) { Serial.printf("[INFO] " format "\n", ##__VA_ARGS__); }

#define LOG_WARN(msg) if (!ConfigLoader::isSilentMode() && ConfigLoader::isAllLogsEnabled()) { Serial.print("[WARN] "); Serial.println(msg); }
#define LOG_WARN_F(format, ...) if (!ConfigLoader::isSilentMode() && ConfigLoader::isAllLogsEnabled()) { Serial.printf("[WARN] " format "\n", ##__VA_ARGS__); }

#define LOG_ERROR(msg) if (!ConfigLoader::isSilentMode()) { Serial.print("[ERROR] "); Serial.println(msg); }
#define LOG_ERROR_F(format, ...) if (!ConfigLoader::isSilentMode()) { Serial.printf("[ERROR] " format "\n", ##__VA_ARGS__); }

// Always enabled logs (critical system messages)
#define LOG_CRITICAL(msg) if (!ConfigLoader::isSilentMode()) { Serial.print("[CRITICAL] "); Serial.println(msg); }
#define LOG_CRITICAL_F(format, ...) if (!ConfigLoader::isSilentMode()) { Serial.printf("[CRITICAL] " format "\n", ##__VA_ARGS__); }

// Service-specific logging macros
#define LOG_TELEGRAM(msg) if (!ConfigLoader::isSilentMode() && (ConfigLoader::isTelegramLogsEnabled() || ConfigLoader::isDebugLogsEnabled() || ConfigLoader::isAllLogsEnabled())) { Serial.print("[TELEGRAM] "); Serial.println(msg); }
#define LOG_TELEGRAM_F(format, ...) if (!ConfigLoader::isSilentMode() && (ConfigLoader::isTelegramLogsEnabled() || ConfigLoader::isDebugLogsEnabled() || ConfigLoader::isAllLogsEnabled())) { Serial.printf("[TELEGRAM] " format "\n", ##__VA_ARGS__); }

#define LOG_DISPLAY(msg) if (!ConfigLoader::isSilentMode() && (ConfigLoader::isDebugLogsEnabled() || ConfigLoader::isAllLogsEnabled())) { Serial.print("[DISPLAY] "); Serial.println(msg); }
#define LOG_DISPLAY_F(format, ...) if (!ConfigLoader::isSilentMode() && (ConfigLoader::isDebugLogsEnabled() || ConfigLoader::isAllLogsEnabled())) { Serial.printf("[DISPLAY] " format "\n", ##__VA_ARGS__); }

#define LOG_HTTP(msg) if (!ConfigLoader::isSilentMode() && (ConfigLoader::isDebugLogsEnabled() || ConfigLoader::isAllLogsEnabled())) { Serial.print("[HTTP] "); Serial.println(msg); }
#define LOG_HTTP_F(format, ...) if (!ConfigLoader::isSilentMode() && (ConfigLoader::isDebugLogsEnabled() || ConfigLoader::isAllLogsEnabled())) { Serial.printf("[HTTP] " format "\n", ##__VA_ARGS__); }

#define LOG_MEMORY(msg) if (!ConfigLoader::isSilentMode() && (ConfigLoader::isDebugLogsEnabled() || ConfigLoader::isAllLogsEnabled())) { Serial.print("[MEMORY] "); Serial.println(msg); }
#define LOG_MEMORY_F(format, ...) if (!ConfigLoader::isSilentMode() && (ConfigLoader::isDebugLogsEnabled() || ConfigLoader::isAllLogsEnabled())) { Serial.printf("[MEMORY] " format "\n", ##__VA_ARGS__); }

#define LOG_NETWORK(msg) if (!ConfigLoader::isSilentMode() && (ConfigLoader::isDebugLogsEnabled() || ConfigLoader::isAllLogsEnabled())) { Serial.print("[NETWORK] "); Serial.println(msg); }
#define LOG_NETWORK_F(format, ...) if (!ConfigLoader::isSilentMode() && (ConfigLoader::isDebugLogsEnabled() || ConfigLoader::isAllLogsEnabled())) { Serial.printf("[NETWORK] " format "\n", ##__VA_ARGS__); }

#define LOG_CONFIG(msg) if (!ConfigLoader::isSilentMode() && (ConfigLoader::isDebugLogsEnabled() || ConfigLoader::isAllLogsEnabled())) { Serial.print("[CONFIG] "); Serial.println(msg); }
#define LOG_CONFIG_F(format, ...) if (!ConfigLoader::isSilentMode() && (ConfigLoader::isDebugLogsEnabled() || ConfigLoader::isAllLogsEnabled())) { Serial.printf("[CONFIG] " format "\n", ##__VA_ARGS__); }

#define LOG_MAIN(msg) if (!ConfigLoader::isSilentMode() && (ConfigLoader::isDebugLogsEnabled() || ConfigLoader::isAllLogsEnabled())) { Serial.print("[MAIN] "); Serial.println(msg); }
#define LOG_MAIN_F(format, ...) if (!ConfigLoader::isSilentMode() && (ConfigLoader::isDebugLogsEnabled() || ConfigLoader::isAllLogsEnabled())) { Serial.printf("[MAIN] " format "\n", ##__VA_ARGS__); }

// Legacy support - these will always print (for backward compatibility during transition)
#define LOG_LEGACY(msg) if (!ConfigLoader::isSilentMode()) { Serial.println(msg); }
#define LOG_LEGACY_F(format, ...) if (!ConfigLoader::isSilentMode()) { Serial.printf(format "\n", ##__VA_ARGS__); }
