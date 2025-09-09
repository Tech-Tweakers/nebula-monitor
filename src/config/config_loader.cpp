#include "config_loader.h"

// Static member definitions
bool ConfigLoader::initialized = false;
String ConfigLoader::configValues[50];
const char* ConfigLoader::configKeys[50];
int ConfigLoader::configCount = 0;

bool ConfigLoader::load() {
  if (initialized) return true;
  
  Serial.println("[CONFIG] Loading configuration...");
  
  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("[CONFIG] ERROR: Failed to initialize SPIFFS!");
    return false;
  }
  
  // Check if config file exists
  if (!SPIFFS.exists("/config.env")) {
    Serial.println("[CONFIG] ERROR: config.env file not found!");
    return false;
  }
  
  // Read config file
  File file = SPIFFS.open("/config.env", "r");
  if (!file) {
    Serial.println("[CONFIG] ERROR: Failed to open config.env!");
    return false;
  }
  
  // Clear arrays
  configCount = 0;
  for (int i = 0; i < 50; i++) {
    configKeys[i] = nullptr;
    configValues[i] = "";
  }
  
  // Parse file line by line
  while (file.available() && configCount < 50) {
    String line = file.readStringUntil('\n');
    line.trim();
    
    // Skip empty lines and comments
    if (line.length() == 0 || line.startsWith("#")) {
      continue;
    }
    
    parseConfigLine(line);
  }
  
  file.close();
  initialized = true;
  
  Serial.printf("[CONFIG] Configuration loaded successfully! (%d settings)\n", configCount);
  return true;
}

void ConfigLoader::cleanup() {
  if (initialized) {
    // Free allocated strings
    for (int i = 0; i < configCount; i++) {
      if (configKeys[i]) {
        free((void*)configKeys[i]);
        configKeys[i] = nullptr;
      }
    }
    
    configCount = 0;
    initialized = false;
    Serial.println("[CONFIG] Configuration cleaned up");
  }
}

void ConfigLoader::parseConfigLine(const String& line) {
  int equalPos = line.indexOf('=');
  if (equalPos == -1) return;
  
  String key = line.substring(0, equalPos);
  String value = line.substring(equalPos + 1);
  
  key.trim();
  value.trim();
  
  // Remove quotes if present
  if (value.startsWith("\"") && value.endsWith("\"")) {
    value = value.substring(1, value.length() - 1);
  }
  
  // Store using strdup for permanent storage
  char* keyCopy = strdup(key.c_str());
  char* valueCopy = strdup(value.c_str());
  
  configKeys[configCount] = keyCopy;
  configValues[configCount] = String(valueCopy);
  configCount++;
  
  Serial.printf("[CONFIG] %s = %s\n", keyCopy, valueCopy);
}

String ConfigLoader::getValue(const char* key, const String& defaultValue) {
  for (int i = 0; i < configCount; i++) {
    if (strcmp(configKeys[i], key) == 0) {
      return configValues[i];
    }
  }
  return defaultValue;
}

// WiFi Configuration
String ConfigLoader::getWifiSSID() {
  return getValue("WIFI_SSID", "Polaris");
}

String ConfigLoader::getWifiPassword() {
  return getValue("WIFI_PASS", "55548502");
}

// Telegram Configuration
String ConfigLoader::getTelegramBotToken() {
  return getValue("TELEGRAM_BOT_TOKEN", "YOUR_BOT_TOKEN_HERE");
}

String ConfigLoader::getTelegramChatId() {
  return getValue("TELEGRAM_CHAT_ID", "YOUR_CHAT_ID_HERE");
}

bool ConfigLoader::isTelegramEnabled() {
  String value = getValue("TELEGRAM_ENABLED", "true");
  return value.equalsIgnoreCase("true");
}

// Alert Configuration
int ConfigLoader::getMaxFailuresBeforeAlert() {
  return getValue("MAX_FAILURES_BEFORE_ALERT", "3").toInt();
}

unsigned long ConfigLoader::getAlertCooldownMs() {
  return getValue("ALERT_COOLDOWN_MS", "300000").toInt();
}

unsigned long ConfigLoader::getAlertRecoveryCooldownMs() {
  return getValue("ALERT_RECOVERY_COOLDOWN_MS", "60000").toInt();
}

// Debug Configuration
bool ConfigLoader::isDebugLogsEnabled() {
  String value = getValue("DEBUG_LOGS_ENABLED", "false");
  return value.equalsIgnoreCase("true");
}

bool ConfigLoader::isTouchLogsEnabled() {
  String value = getValue("TOUCH_LOGS_ENABLED", "false");
  return value.equalsIgnoreCase("true");
}

bool ConfigLoader::isAllLogsEnabled() {
  String value = getValue("ALL_LOGS_ENABLED", "true");
  return value.equalsIgnoreCase("true");
}

// Network Targets
int ConfigLoader::getTargetCount() {
  int count = 0;
  for (int i = 1; i <= 10; i++) {
    String key = "TARGET_" + String(i);
    String value = getValue(key.c_str(), "");
    if (value.length() > 0) {
      count++;
    } else {
      break;
    }
  }
  return count;
}

String ConfigLoader::getTargetName(int index) {
  String key = "TARGET_" + String(index + 1);
  String value = getValue(key.c_str(), "");
  if (value.length() == 0) return "";
  
  int pipe1 = value.indexOf('|');
  if (pipe1 == -1) return "";
  
  return value.substring(0, pipe1);
}

String ConfigLoader::getTargetUrl(int index) {
  String key = "TARGET_" + String(index + 1);
  String value = getValue(key.c_str(), "");
  if (value.length() == 0) return "";
  
  int pipe1 = value.indexOf('|');
  if (pipe1 == -1) return "";
  
  int pipe2 = value.indexOf('|', pipe1 + 1);
  if (pipe2 == -1) return "";
  
  return value.substring(pipe1 + 1, pipe2);
}

String ConfigLoader::getTargetHealthEndpoint(int index) {
  String key = "TARGET_" + String(index + 1);
  String value = getValue(key.c_str(), "");
  if (value.length() == 0) return "";
  
  int pipe1 = value.indexOf('|');
  if (pipe1 == -1) return "";
  
  int pipe2 = value.indexOf('|', pipe1 + 1);
  if (pipe2 == -1) return "";
  
  int pipe3 = value.indexOf('|', pipe2 + 1);
  if (pipe3 == -1) return "";
  
  String endpoint = value.substring(pipe2 + 1, pipe3);
  return endpoint.length() == 0 ? "" : endpoint;
}

String ConfigLoader::getTargetMonitorType(int index) {
  String key = "TARGET_" + String(index + 1);
  String value = getValue(key.c_str(), "");
  if (value.length() == 0) return "PING";
  
  int pipe3 = value.lastIndexOf('|');
  if (pipe3 == -1) return "PING";
  
  return value.substring(pipe3 + 1);
}

// Display Configuration
int ConfigLoader::getDisplayRotation() {
  return getValue("DISPLAY_ROTATION", "2").toInt();
}

int ConfigLoader::getBacklightPin() {
  return getValue("BACKLIGHT_PIN", "27").toInt();
}

// Touch Configuration
int ConfigLoader::getTouchSckPin() {
  return getValue("TOUCH_SCK_PIN", "14").toInt();
}

int ConfigLoader::getTouchMosiPin() {
  return getValue("TOUCH_MOSI_PIN", "13").toInt();
}

int ConfigLoader::getTouchMisoPin() {
  return getValue("TOUCH_MISO_PIN", "12").toInt();
}

int ConfigLoader::getTouchCsPin() {
  return getValue("TOUCH_CS_PIN", "33").toInt();
}

int ConfigLoader::getTouchIrqPin() {
  return getValue("TOUCH_IRQ_PIN", "36").toInt();
}

int ConfigLoader::getTouchXMin() {
  return getValue("TOUCH_X_MIN", "200").toInt();
}

int ConfigLoader::getTouchXMax() {
  return getValue("TOUCH_X_MAX", "3700").toInt();
}

int ConfigLoader::getTouchYMin() {
  return getValue("TOUCH_Y_MIN", "240").toInt();
}

int ConfigLoader::getTouchYMax() {
  return getValue("TOUCH_Y_MAX", "3800").toInt();
}

// Performance Configuration
unsigned long ConfigLoader::getScanIntervalMs() {
  return getValue("SCAN_INTERVAL_MS", "30000").toInt();
}

unsigned long ConfigLoader::getTouchFilterMs() {
  return getValue("TOUCH_FILTER_MS", "500").toInt();
}

unsigned long ConfigLoader::getHttpTimeoutMs() {
  return getValue("HTTP_TIMEOUT_MS", "5000").toInt();
}

// LED Configuration
int ConfigLoader::getLedPinR() {
  return getValue("LED_PIN_R", "16").toInt();
}

int ConfigLoader::getLedPinG() {
  return getValue("LED_PIN_G", "17").toInt();
}

int ConfigLoader::getLedPinB() {
  return getValue("LED_PIN_B", "20").toInt();
}

bool ConfigLoader::isLedActiveHigh() {
  String value = getValue("LED_ACTIVE_HIGH", "false");
  return value.equalsIgnoreCase("true");
}

int ConfigLoader::getLedPwmFreq() {
  return getValue("LED_PWM_FREQ", "5000").toInt();
}

int ConfigLoader::getLedPwmResBits() {
  return getValue("LED_PWM_RES_BITS", "8").toInt();
}

int ConfigLoader::getLedBrightR() {
  return getValue("LED_BRIGHT_R", "32").toInt();
}

int ConfigLoader::getLedBrightG() {
  return getValue("LED_BRIGHT_G", "12").toInt();
}

int ConfigLoader::getLedBrightB() {
  return getValue("LED_BRIGHT_B", "12").toInt();
}

// NTP Configuration
int ConfigLoader::getTimezoneOffset() {
  return getValue("TIMEZONE_OFFSET", "-10800").toInt();
}

String ConfigLoader::getNtpServer() {
  return getValue("NTP_SERVER", "pool.ntp.org");
}

// Health Check Configuration
String ConfigLoader::getHealthCheckHealthyPatterns() {
  return getValue("HEALTH_CHECK_HEALTHY_PATTERNS", "\"status\":\"healthy\",\"status\":\"ok\",\"status\":\"up\",\"status\":\"running\",\"health\":\"ok\",\"health\":\"healthy\",\"health\":\"up\",\"ok\",\"healthy\",\"up\"");
}

String ConfigLoader::getHealthCheckUnhealthyPatterns() {
  return getValue("HEALTH_CHECK_UNHEALTHY_PATTERNS", "\"status\":\"unhealthy\",\"status\":\"down\",\"status\":\"error\",\"status\":\"failed\",\"health\":\"unhealthy\",\"health\":\"down\",\"502 bad gateway\",\"503 service unavailable\",\"504 gateway timeout\",\"500 internal server error\"");
}

bool ConfigLoader::isHealthCheckStrictMode() {
  String value = getValue("HEALTH_CHECK_STRICT_MODE", "false");
  return value.equalsIgnoreCase("true");
}

void ConfigLoader::printAllConfigs() {
  Serial.println("[CONFIG] === All Configuration Settings ===");
  for (int i = 0; i < configCount; i++) {
    if (configKeys[i] != nullptr) {
      Serial.printf("[CONFIG] %s = %s\n", configKeys[i], configValues[i].c_str());
    }
  }
  Serial.println("[CONFIG] ===================================");
}
