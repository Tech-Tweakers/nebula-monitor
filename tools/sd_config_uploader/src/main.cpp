#include <Arduino.h>
#include <SD.h>

// Pin para CS do SD Card (mesmo do projeto principal)
const int SD_CS_PIN = SD_CS_PIN_VALUE;

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("========================================");
  Serial.println("    CONFIG UPLOADER PARA SD CARD");
  Serial.println("========================================");
  
  // Inicializar SD Card
  Serial.println("[UPLOADER] Inicializando SD Card...");
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("[UPLOADER] ERRO: Falha ao inicializar SD Card!");
    Serial.println("[UPLOADER] Verifique se o SD está inserido e funcionando.");
    return;
  }
  Serial.println("[UPLOADER] SD Card inicializado com sucesso!");
  
  // Ler config.env da memória flash (embedded no sketch)
  Serial.println("[UPLOADER] Lendo config.env da memória...");
  
  // O config.env será incluído como string literal
  const char* configContent = R"(
# ===========================================
# ESP32 TFT Network Monitor - Configuration
# ===========================================
# Edite este arquivo para personalizar as configurações
# sem precisar modificar o código fonte

# ===========================================
# WiFi Configuration
# ===========================================
WIFI_SSID=Polaris
WIFI_PASS=55548502

# ===========================================
# Telegram Bot Configuration
# ===========================================
TELEGRAM_BOT_TOKEN=8160557136:AAFvJf0wYywnzyoVWPG8AU1GZrWH9Kdg6yY
TELEGRAM_CHAT_ID=6743862062
TELEGRAM_ENABLED=true

# ===========================================
# Alert Configuration
# ===========================================
MAX_FAILURES_BEFORE_ALERT=3
ALERT_COOLDOWN_MS=300000
ALERT_RECOVERY_COOLDOWN_MS=60000

# ===========================================
# Debug Configuration
# ===========================================
DEBUG_LOGS_ENABLED=false
TOUCH_LOGS_ENABLED=false
TELEGRAM_LOGS_ENABLED=false
ALL_LOGS_ENABLED=false
SILENT_MODE=true

# ===========================================
# Network Targets Configuration
# ===========================================
# Formato: NAME|URL|HEALTH_ENDPOINT|MONITOR_TYPE
# Monitor types: PING, HEALTH_CHECK
# Exemplo: Proxmox HV|http://192.168.1.128:8006/||PING
TARGET_1=Proxmox HV|http://192.168.1.128:8006/||PING
TARGET_2=Router 1|http://192.168.1.1||PING
TARGET_3=Router 2|https://192.168.1.172||PING
TARGET_4=Polaris API|https://rear-describes-eva-intense.trycloudflare.com|/health|HEALTH_CHECK
TARGET_5=Polaris INT|http://ce94eb486673.ngrok-free.app|/health|PING
TARGET_6=Polaris WEB|https://tech-tweakers.github.io/polaris-v2-web||PING

# ===========================================
# Display Configuration
# ===========================================
DISPLAY_ROTATION=2
BACKLIGHT_PIN=27

# ===========================================
# Touch Configuration
# ===========================================
TOUCH_SCK_PIN=14
TOUCH_MOSI_PIN=13
TOUCH_MISO_PIN=12
TOUCH_CS_PIN=33
TOUCH_IRQ_PIN=36

# Touch Calibration
TOUCH_X_MIN=200
TOUCH_X_MAX=3700
TOUCH_Y_MIN=240
TOUCH_Y_MAX=3800

# ===========================================
# Performance Configuration
# ===========================================
SCAN_INTERVAL_MS=90000
TOUCH_FILTER_MS=500
HTTP_TIMEOUT_MS=2000

# ===========================================
# LED (RGB Status) Configuration
# ===========================================
# Default pins and levels for RGB status LED
# Common anode LED: LED_ACTIVE_HIGH=false (active on LOW)
LED_PIN_R=16
LED_PIN_G=17
LED_PIN_B=20
LED_ACTIVE_HIGH=false

# PWM settings
LED_PWM_FREQ=5000
LED_PWM_RES_BITS=8

# Base brightness (0-255) per channel
LED_BRIGHT_R=32
LED_BRIGHT_G=12
LED_BRIGHT_B=12

# ===========================================
# NTP (Network Time Protocol) Configuration
# ===========================================
# Timezone offset in seconds (UTC-3 = -10800)
# Brazil: -10800, UTC: 0, EST: -18000, PST: -28800
TIMEZONE_OFFSET=-10800

# NTP server (optional, defaults to pool.ntp.org)
NTP_SERVER=pool.ntp.org

# ===========================================
# Health Check Configuration
# ===========================================
# Comma-separated patterns for healthy responses (case-insensitive)
HEALTH_CHECK_HEALTHY_PATTERNS="status":"healthy","status":"ok","status":"up","status":"running","health":"ok","health":"healthy","health":"up","ok","healthy","up"

# Comma-separated patterns for unhealthy responses (case-insensitive)
HEALTH_CHECK_UNHEALTHY_PATTERNS="status":"unhealthy","status":"down","status":"error","status":"failed","health":"unhealthy","health":"down","502 bad gateway","503 service unavailable","504 gateway timeout","500 internal server error"

# Strict mode: require explicit health indicators (true/false)
HEALTH_CHECK_STRICT_MODE=false
)";

  // Criar arquivo no SD
  File destFile = SD.open("/config.env", FILE_WRITE);
  if (!destFile) {
    Serial.println("[UPLOADER] ERRO: Não foi possível criar config.env no SD!");
    return;
  }
  
  // Escrever conteúdo no SD
  Serial.println("[UPLOADER] Escrevendo config.env no SD Card...");
  size_t bytesWritten = destFile.print(configContent);
  destFile.close();
  
  Serial.println();
  Serial.printf("[UPLOADER] SUCESSO! %d bytes escritos no SD Card!\n", bytesWritten);
  
  // Verificar se arquivo foi criado corretamente
  if (SD.exists("/config.env")) {
    File verifyFile = SD.open("/config.env", FILE_READ);
    if (verifyFile) {
      Serial.printf("[UPLOADER] Verificação: Arquivo no SD tem %d bytes\n", verifyFile.size());
      verifyFile.close();
    }
  }
  
  Serial.println("[UPLOADER] Configuração copiada com sucesso!");
  Serial.println("[UPLOADER] Agora você pode:");
  Serial.println("  1. Inserir o SD no ESP32 principal");
  Serial.println("  2. Executar o projeto principal");
  Serial.println("  3. O ESP32 vai detectar e sincronizar automaticamente!");
  Serial.println("========================================");
}

void loop() {
  // Nada a fazer aqui
  delay(1000);
}
