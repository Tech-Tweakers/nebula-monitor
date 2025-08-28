#include "scan.hpp"
#include "net.hpp"
#include <Arduino.h>

// Inicialização dos membros estáticos
Target* ScanManager::targets = nullptr;
int ScanManager::targetCount = 0;
int ScanManager::currentTarget = 0;
uint32_t ScanManager::lastScanTime = 0;
uint32_t ScanManager::scanInterval = 10000; // 10 segundos padrão
bool ScanManager::isScanning = false;

// Estados do scan
enum ScanState {
  SCAN_IDLE,
  SCAN_STARTING,
  SCAN_CONNECTING,
  SCAN_PINGING,
  SCAN_COMPLETED
};

static ScanState scanState = SCAN_IDLE;
static uint32_t scanStartTime = 0;
static uint32_t scanTimeout = 0;

bool ScanManager::begin(Target* targetArray, int count) {
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
  scanState = SCAN_IDLE;
  
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
  scanState = SCAN_IDLE;
  Serial.println("[SCAN] Scanning iniciado");
}

void ScanManager::stopScanning() {
  isScanning = false;
  scanState = SCAN_IDLE;
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
    if (currentTarget < targetCount) {
      // Iniciar scan do target atual
      scanState = SCAN_STARTING;
      scanStartTime = now;
      scanTimeout = 2500; // 2.5 segundos timeout
      
      Serial.printf("[SCAN] Iniciando scan de %s...\n", targets[currentTarget].name);
    }
  }
  
  // Máquina de estados para scan não-bloqueante
  switch (scanState) {
    case SCAN_IDLE:
      // Aguardando próximo ciclo
      break;
      
    case SCAN_STARTING:
      // Iniciar conexão HTTP
      scanState = SCAN_CONNECTING;
      Serial.printf("[SCAN] Conectando a %s...\n", targets[currentTarget].name);
      break;
      
    case SCAN_CONNECTING:
      // Simular conexão (não-bloqueante)
      if (now - scanStartTime > 100) { // 100ms para simular conexão
        scanState = SCAN_PINGING;
        Serial.printf("[SCAN] Pingando %s...\n", targets[currentTarget].name);
      }
      break;
      
    case SCAN_PINGING:
      // Simular ping (não-bloqueante)
      if (now - scanStartTime > 200) { // 200ms para simular ping
        // Simular resultado (UP com latência aleatória)
        uint16_t latency = 50 + (random(100)); // 50-150ms
        targets[currentTarget].st = UP;
        targets[currentTarget].lat_ms = latency;
        
        Serial.printf("[SCAN] %s: UP (%d ms)\n", targets[currentTarget].name, latency);
        
        // Próximo target
        currentTarget++;
        scanState = SCAN_COMPLETED;
      }
      break;
      
    case SCAN_COMPLETED:
      // Verificar timeout
      if (now - scanStartTime > scanTimeout) {
        // Timeout - marcar como DOWN
        targets[currentTarget].st = DOWN;
        targets[currentTarget].lat_ms = 0;
        Serial.printf("[SCAN] %s: DOWN (timeout)\n", targets[currentTarget].name);
        
        currentTarget++;
      }
      
      // Se completou o ciclo, reiniciar
      if (currentTarget >= targetCount) {
        currentTarget = 0;
        Serial.println("[SCAN] Ciclo de scan completo");
      }
      
      scanState = SCAN_IDLE;
      lastScanTime = now;
      break;
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

void updateScanner() {
  ScanManager::update();
}
