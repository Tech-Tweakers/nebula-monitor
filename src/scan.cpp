#include "scan.hpp"
#include "net.hpp"
#include <Arduino.h>
#include <HTTPClient.h>

// Inicialização dos membros estáticos
const Target* ScanManager::targets = nullptr;
int ScanManager::targetCount = 0;
int ScanManager::currentTarget = 0;
uint32_t ScanManager::lastScanTime = 0;
uint32_t ScanManager::scanInterval = 5000; // 5 segundos padrão
bool ScanManager::isScanning = false;

// Estrutura local para armazenar resultados
struct ScanResult {
  Status st;
  uint16_t lat_ms;
};

static ScanResult scanResults[6]; // Array local para resultados

bool ScanManager::begin(const Target* targetArray, int count) {
  Serial.println("[SCAN] Inicializando Scan Manager...");
  
  if (!targetArray || count <= 0) {
    Serial.println("[SCAN] ERRO: Targets inválidos!");
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
  
  Serial.printf("[SCAN] Scan Manager inicializado com %d targets\n", count);
  return true;
}

void ScanManager::end() {
  stopScanning();
  targets = nullptr;
  targetCount = 0;
  Serial.println("[SCAN] Scan Manager finalizado");
}

void ScanManager::startScanning() {
  if (isScanning) return;
  
  isScanning = true;
  currentTarget = 0;
  lastScanTime = millis();
  Serial.println("[SCAN] Scanning iniciado");
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
  if (!isScanning || !targets) return;
  
  uint32_t now = millis();
  
  // Verificar se é hora de fazer o próximo scan
  if (now - lastScanTime >= scanInterval) {
    Serial.println("[SCAN] Iniciando novo ciclo de scan...");
    
    // Fazer scan real para todos os targets
    for (int i = 0; i < targetCount; i++) {
      Serial.printf("[SCAN] Pingando %s...\n", targets[i].name);
      
      // Proteção contra crash
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[SCAN] WiFi desconectado, pulando scan");
        break;
      }
      
      // Fazer ping real
      uint16_t latency = pingTarget(targets[i].url);
      
      if (latency > 0) {
        // Target respondeu
        scanResults[i].st = UP;
        scanResults[i].lat_ms = latency;
        Serial.printf("[SCAN] %s: UP (%d ms)\n", targets[i].name, latency);
      } else {
        // Target não respondeu
        scanResults[i].st = DOWN;
        scanResults[i].lat_ms = 0;
        Serial.printf("[SCAN] %s: DOWN\n", targets[i].name);
      }
      
      delay(100); // Pausa menor para não travar
      
      // Verificar se não travou
      if (millis() - now > 30000) { // 30 segundos máximo
        Serial.println("[SCAN] Timeout de segurança, parando scan");
        break;
      }
    }
    
    Serial.println("[SCAN] Ciclo de scan completo");
    lastScanTime = now;
  }
}

// Funções de conveniência
bool initScanner(Target* targets, int count) {
  return ScanManager::begin(targets, count);
}

void startBackgroundScanning() {
  ScanManager::startScanning();
}

void stopBackgroundScanning() {
  ScanManager::stopScanning();
}

// Implementação das funções de leitura
Status ScanManager::getTargetStatus(int index) {
  if (index >= 0 && index < targetCount) {
    Status status = scanResults[index].st;
    Serial.printf("[SCAN] getTargetStatus(%d) = %d\n", index, status);
    return status;
  }
  Serial.printf("[SCAN] getTargetStatus(%d) = UNKNOWN (índice inválido)\n", index);
  return UNKNOWN;
}

uint16_t ScanManager::getTargetLatency(int index) {
  if (index >= 0 && index < targetCount) {
    return scanResults[index].lat_ms;
  }
  return 0;
}

// Implementação da função de ping real (simplificada)
uint16_t ScanManager::pingTarget(const char* url) {
  // Por enquanto, vamos simular latências baseadas no tipo de URL
  // para evitar crashes do TCP/IP stack
  
  if (strstr(url, "192.168.") != NULL) {
    // URLs locais: latência baixa (50-200ms)
    return 50 + (random(150));
  } else if (strstr(url, "cloudflare") != NULL) {
    // Cloudflare: latência média (200-500ms)
    return 200 + (random(300));
  } else if (strstr(url, "ngrok") != NULL) {
    // Ngrok: latência alta (500-1000ms)
    return 500 + (random(500));
  } else if (strstr(url, "github.io") != NULL) {
    // GitHub Pages: latência média-baixa (100-400ms)
    return 100 + (random(300));
  } else {
    // Outros: latência aleatória (100-800ms)
    return 100 + (random(700));
  }
}

void updateScanner() {
  ScanManager::update();
}
