#pragma once
#include <Arduino.h>
#include "config.hpp"

class TelegramAlerts {
private:
  static bool isEnabled;
  static String botToken;
  static String chatId;
  static AlertState alertStates[6]; // Array para controlar alertas de cada target
  static bool sendingMessage; // Flag para indicar se está enviando mensagem
  
public:
  // Inicialização
  static bool begin();
  // static void end(); - removida (não utilizada)
  
  // Configuração - funções removidas (não utilizadas)
  
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
  static bool isSendingMessage() { return sendingMessage; }
  static int getFailureCount(int targetIndex);
  // static void resetFailureCount(int targetIndex); - removida (não utilizada)
  static bool hasActiveAlerts(); // Verifica se há alertas ativos
};

// Funções de conveniência
bool initTelegramAlerts();
void updateTelegramAlert(int targetIndex, Status status, uint16_t latency);
void sendTestTelegramAlert(Target* targets, int targetCount);

