#pragma once
#include "config.hpp"

class ScanManager {
private:
  static const Target* targets;
  static int targetCount;
  static int currentTarget;
  static uint32_t lastScanTime;
  static uint32_t scanInterval;
  static bool isScanning;
  // Callbacks
  static void (*onScanStart)();
  static void (*onScanComplete)();
  
public:
  static bool begin(const Target* targetArray, int count);
  // static void end(); - removida (não utilizada)
  static void startScanning();
  static void stopScanning();
  static void update(); // Chamar no loop principal
  static void setInterval(uint32_t interval_ms);
  static bool isActive() { return isScanning; }
  static int getCurrentTarget() { return currentTarget; }
  static int getTotalTargets() { return targetCount; }
  static void setCallbacks(void (*onStart)(), void (*onComplete)()) { onScanStart = onStart; onScanComplete = onComplete; }
  
  // Funções para ler resultados
  static Status getTargetStatus(int index);
  static uint16_t getTargetLatency(int index);
  
  // Função para fazer ping real
  static uint16_t pingTarget(const char* url);
  
  // Função para health check via API
  static uint16_t healthCheckTarget(const char* base_url, const char* health_endpoint);
  static String getHealthCheckPayload(const char* url, uint16_t timeout);
};

// Funções de conveniência
bool initScanner(Target* targets, int count);
// Funções removidas: startBackgroundScanning(), stopBackgroundScanning(), updateScanner() - não utilizadas
