#pragma once

#include <TFT_eSPI.h>
#include <SPI.h>

// ----------------- Hardware base -----------------
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

// WiFi credentials
#define WIFI_SSID "Polaris"
#define WIFI_PASS "55548502"

// Telegram Bot Configuration
#define TELEGRAM_BOT_TOKEN "8160557136:AAFvJf0wYywnzyoVWPG8AU1GZrWH9Kdg6yY"
#define TELEGRAM_CHAT_ID "6743862062"
#define TELEGRAM_ENABLED true

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

// Configurações de alertas
#define MAX_FAILURES_BEFORE_ALERT 3
#define ALERT_COOLDOWN_MS 300000  // 5 minutos entre alertas
#define ALERT_RECOVERY_COOLDOWN_MS 60000  // 1 minuto para alerta de recuperação

// Configurações de debug
#define DEBUG_LOGS_ENABLED false  // true = logs ativos, false = logs desabilitados
#define TOUCH_LOGS_ENABLED false  // true = logs de touch ativos, false = logs de touch desabilitados

// Macro para logs condicionais
#if DEBUG_LOGS_ENABLED
  #define DEBUG_LOG(x) Serial.print(x)
  #define DEBUG_LOGF(x, ...) Serial.printf(x, __VA_ARGS__)
  #define DEBUG_LOGLN(x) Serial.println(x)
#else
  #define DEBUG_LOG(x)
  #define DEBUG_LOGF(x, ...)
  #define DEBUG_LOGLN(x)
#endif

// Macro para logs de touch condicionais
#if TOUCH_LOGS_ENABLED
  #define TOUCH_LOG(x) Serial.print(x)
  #define TOUCH_LOGF(x, ...) Serial.printf(x, __VA_ARGS__)
  #define TOUCH_LOGLN(x) Serial.println(x)
#else
  #define TOUCH_LOG(x)
  #define TOUCH_LOGF(x, ...)
  #define TOUCH_LOGLN(x)
#endif
