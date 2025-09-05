#include "scan.hpp"
#include "net.hpp"
#include <Arduino.h>
#include <HTTPClient.h>

// Inicialização dos membros estáticos
const Target* ScanManager::targets = nullptr;
int ScanManager::targetCount = 0;
int ScanManager::currentTarget = 0;
uint32_t ScanManager::lastScanTime = 0;
uint32_t ScanManager::scanInterval = 30000; // 60 segundos padrão
bool ScanManager::isScanning = false;
void (*ScanManager::onScanStart)() = nullptr;
void (*ScanManager::onScanComplete)() = nullptr;
void (*ScanManager::onTargetScanned)(int, Status, uint16_t) = nullptr;

// Estrutura local para armazenar resultados
struct ScanResult {
  Status st;
  uint16_t lat_ms;
};

static ScanResult scanResults[6]; // Array local para resultados

bool ScanManager::begin(const Target* targetArray, int count) {
  Serial.println("[SCAN] Initializing Scan Manager...");
  
  if (!targetArray || count <= 0) {
    Serial.println("[SCAN] ERROR: Invalid targets!");
    return false;
  }
  
  targets = targetArray;
  targetCount = count;
  currentTarget = 0;
  lastScanTime = 0;
  isScanning = false;
  
  // Inicializar scanResults com valores padrão
  for (int i = 0; i < count; i++) {
    scanResults[i].st = UNKNOWN;
    scanResults[i].lat_ms = 0;
  }
  
  Serial.printf("[SCAN] Scan Manager initialized with %d targets\n", count);
  return true;
}

// Função removida: end() - não utilizada

void ScanManager::startScanning() {
  if (isScanning) return;
  
  isScanning = true;
  currentTarget = 0;
  lastScanTime = millis();
  Serial.println("[SCAN] Scanning iniciado");
  if (onScanStart) onScanStart();
}

void ScanManager::stopScanning() {
  isScanning = false;
  Serial.println("[SCAN] Scanning parado");
}

void ScanManager::setInterval(uint32_t interval_ms) {
  scanInterval = interval_ms;
  Serial.printf("[SCAN] Intervalo definido para %lu ms\n", interval_ms);
}

void ScanManager::update() {
  if (!targets) return;
  
  uint32_t now = millis();
  
  // Verificar se é hora de fazer o próximo scan
  if (now - lastScanTime >= scanInterval) {
    Serial.println("[SCAN] Starting new scan cycle...");
    isScanning = true; // Mark scan as active
    if (onScanStart) onScanStart();
    
    // Fazer scan real para todos os targets
    for (int i = 0; i < targetCount; i++) {
      Serial.printf("[SCAN] Verificando %s (tipo: %s)...\n", 
                   targets[i].name, 
                   targets[i].monitor_type == HEALTH_CHECK ? "HEALTH_CHECK" : "PING");
      
      // Proteção contra crash
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[SCAN] WiFi desconectado, pulando scan");
        break;
      }
      
      uint16_t latency = 0;
      
      // Escolher tipo de verificação baseado na configuração
      if (targets[i].monitor_type == HEALTH_CHECK) {
        // Health check via API endpoint
        Serial.printf("[SCAN] Performing health check for %s...\n", targets[i].name);
        latency = healthCheckTarget(targets[i].url, targets[i].health_endpoint);
      } else {
        // Ping simples
        Serial.printf("[SCAN] Performing ping for %s...\n", targets[i].name);
        latency = pingTarget(targets[i].url);
      }
      
      if (latency > 0) {
        // Target respondeu
        scanResults[i].st = UP;
        scanResults[i].lat_ms = latency;
        Serial.printf("[SCAN] %s: UP (%d ms) - %s\n", 
                     targets[i].name, latency,
                     targets[i].monitor_type == HEALTH_CHECK ? "Health OK" : "Ping OK");
      } else {
        // Target não respondeu
        scanResults[i].st = DOWN;
        scanResults[i].lat_ms = 0;
        Serial.printf("[SCAN] %s: DOWN - %s\n", 
                     targets[i].name,
                     targets[i].monitor_type == HEALTH_CHECK ? "Health FAIL" : "Ping FAIL");
      }

      // Callback por target para atualização imediata de UI/LED/footer
      if (onTargetScanned) onTargetScanned(i, scanResults[i].st, scanResults[i].lat_ms);
      
      delay(200); // Pausa menor entre targets
      
      // Verificar se não travou
      if (millis() - now > 30000) { // 30 segundos máximo
        Serial.println("[SCAN] Safety timeout, stopping scan");
        break;
      }
    }
    
    Serial.println("[SCAN] Scan cycle complete");
    lastScanTime = now;
    isScanning = false; // Mark scan as complete
    if (onScanComplete) onScanComplete();
  }
}

// Funções de conveniência
bool initScanner(Target* targets, int count) {
  return ScanManager::begin(targets, count);
}

// Funções removidas: startBackgroundScanning(), stopBackgroundScanning() - não utilizadas

// Implementação das funções de leitura
Status ScanManager::getTargetStatus(int index) {
  if (index >= 0 && index < targetCount) {
    Status status = scanResults[index].st;
    DEBUG_LOGF("[SCAN] getTargetStatus(%d) = %d\n", index, status);
    return status;
  }
  DEBUG_LOGF("[SCAN] getTargetStatus(%d) = UNKNOWN (índice inválido)\n", index);
  return UNKNOWN;
}

uint16_t ScanManager::getTargetLatency(int index) {
  if (index >= 0 && index < targetCount) {
    return scanResults[index].lat_ms;
  }
  return 0;
}

// Implementação da função de ping real
uint16_t ScanManager::pingTarget(const char* url) {
  // Usar a função httpPing do Net para fazer ping real
  return Net::httpPing(url, 5000); // 5 segundos de timeout
}

// Nova função para health check via API
uint16_t ScanManager::healthCheckTarget(const char* base_url, const char* health_endpoint) {
  if (!health_endpoint) {
    return 0; // Sem endpoint de health, retorna 0 (DOWN)
  }
  
  // Construir URL completa
  String full_url = String(base_url);
  if (full_url.endsWith("/") && health_endpoint[0] == '/') {
    full_url = full_url.substring(0, full_url.length() - 1); // Remove trailing slash
  } else if (!full_url.endsWith("/") && health_endpoint[0] != '/') {
    full_url += "/"; // Adiciona slash se necessário
  }
  full_url += health_endpoint;
  
  Serial.printf("[HEALTH] Checking health endpoint: %s\n", full_url.c_str());
  
  // Para endpoints ngrok, tentar com timeout maior e retry
  uint16_t timeout = 7000;
  if (strstr(base_url, "ngrok-free.app")) {
    timeout = 10000; // 10 segundos para ngrok
  }
  
  // Fazer requisição HTTP para o health endpoint
  uint16_t latency = Net::httpPing(full_url.c_str(), timeout);
  
  // Se falhou e é ngrok, tentar HTTP como fallback
  if (latency == 0 && strstr(base_url, "ngrok-free.app")) {
    Serial.printf("[HEALTH] HTTPS failed, trying HTTP as fallback...\n");
    String http_url = full_url;
    http_url.replace("https://", "http://");
    latency = Net::httpPing(http_url.c_str(), timeout);
    if (latency > 0) {
      Serial.printf("[HEALTH] HTTP fallback worked: %d ms\n", latency);
    }
  }
  
  if (latency > 0) {
    // Verificar se o payload JSON indica status healthy
    // Para isso, precisamos fazer uma requisição GET completa para obter o payload
    String payload = getHealthCheckPayload(full_url.c_str(), timeout);
    if (payload.length() > 0) {
      // Verificar se o payload contém status healthy
      if (payload.indexOf("\"status\":\"healthy\"") > 0) {
        Serial.printf("[HEALTH] Health check OK: %d ms (status: healthy)\n", latency);
        return latency;
      } else {
        Serial.printf("[HEALTH] Health check FAILED: status is not healthy\n");
        Serial.printf("[HEALTH] Payload: %s\n", payload.c_str());
        return 0;
      }
    } else {
      // Se não conseguiu obter payload, considerar como OK se HTTP respondeu
      Serial.printf("[HEALTH] Health check OK: %d ms (without payload verification)\n", latency);
      return latency;
    }
  } else {
    Serial.printf("[HEALTH] Health check FAILED\n");
    return 0;
  }
}

// Função para obter o payload completo do health check
String ScanManager::getHealthCheckPayload(const char* url, uint16_t timeout) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[HEALTH] WiFi not connected to get payload");
    return "";
  }
  
  HTTPClient http;
  http.setTimeout(timeout);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  
  if (strncmp(url, "https://", 8) == 0) {
    WiFiClientSecure client;
    client.setInsecure();
    if (strstr(url, "ngrok-free.app")) {
      // Configurações específicas para ngrok
      client.setCACert(nullptr);  // Não usar CA cert
      client.setInsecure();       // Ignorar validação SSL
    }
    client.setTimeout((timeout + 999) / 1000);
    
    if (http.begin(client, url)) {
      http.addHeader("User-Agent", "NebulaWatch/1.0");
      http.addHeader("Accept", "application/json");
      if (strstr(url, "ngrok-free.app")) {
        http.addHeader("ngrok-skip-browser-warning", "true");
      }
      
      int code = http.GET();
      if (code > 0) {
        String payload = http.getString();
        http.end();
        Serial.printf("[HEALTH] Payload obtido: %s\n", payload.c_str());
        return payload;
      }
    }
  } else {
    WiFiClient client;
    client.setTimeout((timeout + 999) / 1000);
    
    if (http.begin(client, url)) {
      http.addHeader("User-Agent", "NebulaWatch/1.0");
      http.addHeader("Accept", "application/json");
      if (strstr(url, "ngrok-free.app")) {
        http.addHeader("ngrok-skip-browser-warning", "true");
      }
      
      int code = http.GET();
      if (code > 0) {
        String payload = http.getString();
        http.end();
        Serial.printf("[HEALTH] Payload obtido: %s\n", payload.c_str());
        return payload;
      }
    }
  }
  
  http.end();
  Serial.println("[HEALTH] Failed to get payload");
  return "";
}
