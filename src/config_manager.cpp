#include "config_manager.hpp"

// Inicialização dos membros estáticos
bool ConfigManager::initialized = false;
String ConfigManager::configValues[50];
const char* ConfigManager::configKeys[50];
int ConfigManager::configCount = 0;

bool ConfigManager::begin() {
  if (initialized) return true;
  
  Serial.println("[CONFIG] Inicializando ConfigManager...");
  
  // Inicializar SPIFFS
  Serial.println("[CONFIG] Inicializando SPIFFS...");
  if (!SPIFFS.begin(true)) {
    Serial.println("[CONFIG] Erro ao inicializar SPIFFS!");
    return false;
  }
  Serial.println("[CONFIG] SPIFFS inicializado com sucesso!");
  
  // Listar arquivos no SPIFFS
  Serial.println("[CONFIG] Listando arquivos no SPIFFS:");
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file) {
    Serial.printf("[CONFIG] Arquivo: %s (tamanho: %d bytes)\n", file.name(), file.size());
    file = root.openNextFile();
  }
  
  // Verificar se arquivo existe
  Serial.println("[CONFIG] Verificando se config.env existe...");
  if (!SPIFFS.exists("/config.env")) {
    Serial.println("[CONFIG] Arquivo config.env não encontrado!");
    return false;
  }
  Serial.println("[CONFIG] Arquivo config.env encontrado!");
  
  // Ler arquivo de configuração
  file = SPIFFS.open("/config.env", "r");
  if (!file) {
    Serial.println("[CONFIG] Erro ao abrir config.env!");
    return false;
  }
  
  // Limpar arrays
  configCount = 0;
  for (int i = 0; i < 50; i++) {
    configKeys[i] = nullptr;
    configValues[i] = "";
  }
  
  // Ler linha por linha
  while (file.available() && configCount < 50) {
    String line = file.readStringUntil('\n');
    line.trim();
    
    // Pular linhas vazias e comentários
    if (line.length() == 0 || line.startsWith("#")) {
      continue;
    }
    
    parseConfigLine(line);
  }
  
  file.close();
  initialized = true;
  
  Serial.printf("[CONFIG] ConfigManager inicializado com %d configurações\n", configCount);
  return true;
}

void ConfigManager::end() {
  initialized = false;
  configCount = 0;
  Serial.println("[CONFIG] ConfigManager finalizado");
}

void ConfigManager::parseConfigLine(const String& line) {
  int equalPos = line.indexOf('=');
  if (equalPos == -1) return;
  
  String key = line.substring(0, equalPos);
  String value = line.substring(equalPos + 1);
  
  key.trim();
  value.trim();
  
  // Remover aspas se existirem
  if (value.startsWith("\"") && value.endsWith("\"")) {
    value = value.substring(1, value.length() - 1);
  }
  
  // Usar strdup para criar cópias permanentes das strings
  char* keyCopy = strdup(key.c_str());
  char* valueCopy = strdup(value.c_str());
  
  configKeys[configCount] = keyCopy;
  configValues[configCount] = String(valueCopy);
  configCount++;
  
  Serial.printf("[CONFIG] %s = %s\n", keyCopy, valueCopy);
}

String ConfigManager::getValue(const char* key, const String& defaultValue) {
  for (int i = 0; i < configCount; i++) {
    if (strcmp(configKeys[i], key) == 0) {
      return configValues[i];
    }
  }
  return defaultValue;
}

// WiFi
String ConfigManager::getWifiSSID() {
  return getValue("WIFI_SSID", "Polaris");
}

String ConfigManager::getWifiPass() {
  return getValue("WIFI_PASS", "55548502");
}

// Telegram
String ConfigManager::getTelegramBotToken() {
  return getValue("TELEGRAM_BOT_TOKEN", "YOUR_BOT_TOKEN_HERE");
}

String ConfigManager::getTelegramChatId() {
  return getValue("TELEGRAM_CHAT_ID", "YOUR_CHAT_ID_HERE");
}

bool ConfigManager::isTelegramEnabled() {
  String value = getValue("TELEGRAM_ENABLED", "true");
  return value.equalsIgnoreCase("true");
}

// Alerts
int ConfigManager::getMaxFailuresBeforeAlert() {
  return getValue("MAX_FAILURES_BEFORE_ALERT", "3").toInt();
}

unsigned long ConfigManager::getAlertCooldownMs() {
  return getValue("ALERT_COOLDOWN_MS", "300000").toInt();
}

unsigned long ConfigManager::getAlertRecoveryCooldownMs() {
  return getValue("ALERT_RECOVERY_COOLDOWN_MS", "60000").toInt();
}

// Debug
bool ConfigManager::isDebugLogsEnabled() {
  String value = getValue("DEBUG_LOGS_ENABLED", "false");
  return value.equalsIgnoreCase("true");
}

bool ConfigManager::isTouchLogsEnabled() {
  String value = getValue("TOUCH_LOGS_ENABLED", "false");
  return value.equalsIgnoreCase("true");
}

bool ConfigManager::isAllLogsEnabled() {
  String value = getValue("ALL_LOGS_ENABLED", "true");
  return value.equalsIgnoreCase("true");
}

// Network Targets
int ConfigManager::getTargetCount() {
  int count = 0;
  for (int i = 1; i <= 10; i++) {
    String key = "TARGET_" + String(i);
    String value = getValue(key.c_str(), "");
    Serial.printf("[CONFIG] Verificando %s: '%s' (length=%d)\n", key.c_str(), value.c_str(), value.length());
    if (value.length() > 0) {
      count++;
    } else {
      break;
    }
  }
  Serial.printf("[CONFIG] Total de targets encontrados: %d\n", count);
  return count;
}

String ConfigManager::getTargetName(int index) {
  String key = "TARGET_" + String(index + 1);
  String value = getValue(key.c_str(), "");
  if (value.length() == 0) return "";
  
  int pipe1 = value.indexOf('|');
  if (pipe1 == -1) return "";
  
  return value.substring(0, pipe1);
}

String ConfigManager::getTargetUrl(int index) {
  String key = "TARGET_" + String(index + 1);
  String value = getValue(key.c_str(), "");
  if (value.length() == 0) return "";
  
  int pipe1 = value.indexOf('|');
  if (pipe1 == -1) return "";
  
  int pipe2 = value.indexOf('|', pipe1 + 1);
  if (pipe2 == -1) return "";
  
  return value.substring(pipe1 + 1, pipe2);
}

String ConfigManager::getTargetHealthEndpoint(int index) {
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

String ConfigManager::getTargetMonitorType(int index) {
  String key = "TARGET_" + String(index + 1);
  String value = getValue(key.c_str(), "");
  if (value.length() == 0) return "PING";
  
  int pipe3 = value.lastIndexOf('|');
  if (pipe3 == -1) return "PING";
  
  return value.substring(pipe3 + 1);
}

// Display
int ConfigManager::getDisplayRotation() {
  return getValue("DISPLAY_ROTATION", "2").toInt();
}

int ConfigManager::getBacklightPin() {
  return getValue("BACKLIGHT_PIN", "27").toInt();
}

// Touch
int ConfigManager::getTouchSckPin() {
  return getValue("TOUCH_SCK_PIN", "14").toInt();
}

int ConfigManager::getTouchMosiPin() {
  return getValue("TOUCH_MOSI_PIN", "13").toInt();
}

int ConfigManager::getTouchMisoPin() {
  return getValue("TOUCH_MISO_PIN", "12").toInt();
}

int ConfigManager::getTouchCsPin() {
  return getValue("TOUCH_CS_PIN", "33").toInt();
}

int ConfigManager::getTouchIrqPin() {
  return getValue("TOUCH_IRQ_PIN", "36").toInt();
}

int ConfigManager::getTouchXMin() {
  return getValue("TOUCH_X_MIN", "200").toInt();
}

int ConfigManager::getTouchXMax() {
  return getValue("TOUCH_X_MAX", "3700").toInt();
}

int ConfigManager::getTouchYMin() {
  return getValue("TOUCH_Y_MIN", "240").toInt();
}

int ConfigManager::getTouchYMax() {
  return getValue("TOUCH_Y_MAX", "3800").toInt();
}

// Performance
unsigned long ConfigManager::getScanIntervalMs() {
  return getValue("SCAN_INTERVAL_MS", "30000").toInt();
}

unsigned long ConfigManager::getTouchFilterMs() {
  return getValue("TOUCH_FILTER_MS", "500").toInt();
}

unsigned long ConfigManager::getHttpTimeoutMs() {
  return getValue("HTTP_TIMEOUT_MS", "5000").toInt();
}

// Debug
void ConfigManager::printAllConfigs() {
  Serial.println("[CONFIG] === Todas as configurações ===");
  for (int i = 0; i < configCount; i++) {
    if (configKeys[i] != nullptr) {
      Serial.printf("[CONFIG] %s = %s\n", configKeys[i], configValues[i].c_str());
    }
  }
  Serial.println("[CONFIG] ==============================");
}

// LED (RGB Status)
int ConfigManager::getLedPinR() {
  return getValue("LED_PIN_R", "16").toInt();
}

int ConfigManager::getLedPinG() {
  return getValue("LED_PIN_G", "17").toInt();
}

int ConfigManager::getLedPinB() {
  return getValue("LED_PIN_B", "20").toInt();
}

bool ConfigManager::isLedActiveHigh() {
  String v = getValue("LED_ACTIVE_HIGH", "false");
  return v.equalsIgnoreCase("true");
}

int ConfigManager::getLedPwmFreq() {
  return getValue("LED_PWM_FREQ", "5000").toInt();
}

int ConfigManager::getLedPwmResBits() {
  return getValue("LED_PWM_RES_BITS", "8").toInt();
}

int ConfigManager::getLedBrightR() {
  return getValue("LED_BRIGHT_R", "32").toInt();
}

int ConfigManager::getLedBrightG() {
  return getValue("LED_BRIGHT_G", "12").toInt();
}

int ConfigManager::getLedBrightB() {
  return getValue("LED_BRIGHT_B", "12").toInt();
}
