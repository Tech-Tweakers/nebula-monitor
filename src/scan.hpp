#pragma once
#include "config.hpp"

class ScanManager {
private:
  static Target* targets;
  static int targetCount;
  static int currentTarget;
  static uint32_t lastScanTime;
  static uint32_t scanInterval;
  static bool isScanning;
  
public:
  static bool begin(Target* targetArray, int count);
  static void end();
  static void startScanning();
  static void stopScanning();
  static void update(); // Chamar no loop principal
  static void setInterval(uint32_t interval_ms);
  static bool isActive() { return isScanning; }
  static int getCurrentTarget() { return currentTarget; }
  static int getTotalTargets() { return targetCount; }
};

// Funções de conveniência
bool initScanner(Target* targets, int count);
void startBackgroundScanning();
void stopBackgroundScanning();
void updateScanner();
