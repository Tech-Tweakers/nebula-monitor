#pragma once
#include <Arduino.h>
#include "config.hpp"

class TelegramAlerts {
private:
  static bool isEnabled;
  static String botToken;
  static String chatId;
  static AlertState alertStates[6]; // Array para controlar alertas de cada target
  
public:
  // Inicialização
  static bool begin();
  static void end();
  
  // Configuração
  static void setBotToken(const char* token);
  static void setChatId(const char* id);
  static void enable(bool enabled = true);
  
  // Controle de alertas
  static void updateTargetStatus(int targetIndex, Status newStatus, uint16_t latency);
  static void sendAlert(int targetIndex, const char* targetName, Status status, uint16_t latency);
  static void sendRecoveryAlert(int targetIndex, const char* targetName, uint16_t latency);
  
  // Funções auxiliares
  static String formatAlertMessage(const char* targetName, Status status, uint16_t latency, bool isRecovery = false, unsigned long totalDowntime = 0);
  static bool sendMessage(const String& message);
  static bool isTimeForAlert(int targetIndex, bool isRecovery = false);
  static bool isHealthCheckHealthy(const String& response);
  

  
  // Status
  static bool isActive() { 
    bool active = isEnabled && botToken.length() > 0 && chatId.length() > 0;
    if (!active) {
      Serial.printf("[TELEGRAM] isActive=false: isEnabled=%s, botToken.length=%d, chatId.length=%d\n", 
                   isEnabled ? "true" : "false", botToken.length(), chatId.length());
    }
    return active;
  }
  static int getFailureCount(int targetIndex);
  static void resetFailureCount(int targetIndex);
};

// Funções de conveniência
bool initTelegramAlerts();
void updateTelegramAlert(int targetIndex, Status status, uint16_t latency);
void sendTestTelegramAlert();

