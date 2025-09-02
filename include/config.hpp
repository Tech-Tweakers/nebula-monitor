#pragma once

#include <TFT_eSPI.h>
#include <SPI.h>
#include <stdarg.h>
#include "config_manager.hpp"

// ----------------- Hardware base -----------------
// Valores padrão - serão sobrescritos pelo ConfigManager
static constexpr uint8_t ROT    = 2;   // rotação landscape (como no tutorial)
static constexpr int     BL_PIN = 27;  // backlight

// Touch (XPT2046) - pinagem CORRETA da CYD
static constexpr int T_SCK  = 14;  // T_CLK (HSPI)
static constexpr int T_MOSI = 13;  // T_DIN (HSPI)
static constexpr int T_MISO = 12;  // T_OUT (HSPI)
static constexpr int T_CS   = 33;  // T_CS
static constexpr int T_IRQ  = 36;  // T_IRQ

// Calibração da CYD (do tutorial)
static constexpr int RAW_X_MIN = 200;
static constexpr int RAW_X_MAX = 3700;
static constexpr int RAW_Y_MIN = 240;
static constexpr int RAW_Y_MAX = 3800;

// WiFi credentials - serão lidos do config.env
#define WIFI_SSID ConfigManager::getWifiSSID().c_str()
#define WIFI_PASS ConfigManager::getWifiPass().c_str()

// Telegram Bot Configuration - serão lidos do config.env
#define TELEGRAM_BOT_TOKEN ConfigManager::getTelegramBotToken().c_str()
#define TELEGRAM_CHAT_ID ConfigManager::getTelegramChatId().c_str()
#define TELEGRAM_ENABLED ConfigManager::isTelegramEnabled()

// RGB color conversion macro
#define RGB(r,g,b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))

// Cores base (usando TFT_eSPI)
#define COL_BG    TFT_BLACK
#define COL_BAR   TFT_DARKGREY
#define COL_TXT   TFT_WHITE
#define COL_UP    RGB(0,140,60)
#define COL_DOWN  TFT_RED
#define COL_UNK   TFT_DARKGREY

// Estados (compartilhado entre main/ui)
enum Status : uint8_t { UNKNOWN=0, UP=1, DOWN=2 };

// Tipos de monitoramento
enum MonitorType : uint8_t { PING=0, HEALTH_CHECK=1 };

// Target struct para network scanning
struct Target {
  const char* name;
  const char* url;
  const char* health_endpoint;  // Endpoint para health check (opcional)
  MonitorType monitor_type;     // Tipo de monitoramento
  Status  st;
  uint16_t lat_ms;
};

// Info que desenhamos em cada tile
struct TileInfo {
  const char* name;
  Status      st;
  uint16_t    lat_ms;
};

// Estrutura para controle de alertas
struct AlertState {
  uint8_t failure_count;      // Contador de falhas consecutivas
  uint8_t last_status;        // Último status conhecido
  unsigned long last_alert;   // Timestamp do último alerta
  bool alert_sent;            // Se já foi enviado alerta
  unsigned long downtime_start;  // Timestamp quando ficou DOWN
};

// Configurações de alertas - serão lidas do config.env
#define MAX_FAILURES_BEFORE_ALERT ConfigManager::getMaxFailuresBeforeAlert()
#define ALERT_COOLDOWN_MS ConfigManager::getAlertCooldownMs()  // 5 minutos entre alertas
#define ALERT_RECOVERY_COOLDOWN_MS ConfigManager::getAlertRecoveryCooldownMs()  // 1 minuto para alerta de recuperação

// Configurações de debug - serão lidas do config.env
// Usar variáveis estáticas em vez de macros para permitir chamadas de função
static bool DEBUG_LOGS_ENABLED = false;  // Será inicializado pelo ConfigManager
static bool TOUCH_LOGS_ENABLED = false;  // Será inicializado pelo ConfigManager  
static bool ALL_LOGS_ENABLED = true;     // Será inicializado pelo ConfigManager

// Funções para logs condicionais (runtime)
inline void DEBUG_LOG(const String& x) { if (DEBUG_LOGS_ENABLED) Serial.print(x); }
inline void DEBUG_LOGF(const char* format, ...) { 
  if (DEBUG_LOGS_ENABLED) {
    va_list args;
    va_start(args, format);
    Serial.printf(format, args);
    va_end(args);
  }
}
inline void DEBUG_LOGLN(const String& x) { if (DEBUG_LOGS_ENABLED) Serial.println(x); }

// Funções para logs de touch condicionais
inline void TOUCH_LOG(const String& x) { if (TOUCH_LOGS_ENABLED) Serial.print(x); }
inline void TOUCH_LOGF(const char* format, ...) { 
  if (TOUCH_LOGS_ENABLED) {
    va_list args;
    va_start(args, format);
    Serial.printf(format, args);
    va_end(args);
  }
}
inline void TOUCH_LOGLN(const String& x) { if (TOUCH_LOGS_ENABLED) Serial.println(x); }

// Funções para TODOS os logs condicionais (MÁXIMA PERFORMANCE)
inline void LOG(const String& x) { if (ALL_LOGS_ENABLED) Serial.print(x); }
inline void LOGF(const char* format, ...) { 
  if (ALL_LOGS_ENABLED) {
    va_list args;
    va_start(args, format);
    Serial.printf(format, args);
    va_end(args);
  }
}
inline void LOGLN(const String& x) { if (ALL_LOGS_ENABLED) Serial.println(x); }
