#pragma once
#include "core/domain/alert.h"
#include "ssl_mutex_manager.h"
#include "ntp_service.h"
#include <Arduino.h>

class TelegramService {
private:
  String botToken;
  String chatId;
  bool enabled;
  bool sendingMessage;
  Alert* alerts[6]; // Array of alert objects
  
  // Configuration
  static const uint8_t MAX_FAILURES_BEFORE_ALERT = 3;
  static const unsigned long ALERT_COOLDOWN_MS = 300000; // 5 minutes
  static const unsigned long ALERT_RECOVERY_COOLDOWN_MS = 60000; // 1 minute
  
public:
  TelegramService();
  ~TelegramService();
  
  // Initialization
  bool initialize(const String& botToken, const String& chatId, bool enabled = true);
  
  // Alert management
  void updateTargetStatus(int targetIndex, Status newStatus, uint16_t latency, const String& targetName);
  void sendAlert(int targetIndex, const String& targetName, Status status, uint16_t latency);
  void sendRecoveryAlert(int targetIndex, const String& targetName, uint16_t latency);
  void sendTestMessage(const String* targetNames, int targetCount);
  
  // Status
  bool isActive() const;
  bool isSendingMessage() const { return sendingMessage; }
  bool hasActiveAlerts() const;
  
  // Alert management
  int getFailureCount(int targetIndex) const;
  
private:
  // Message formatting
  String formatAlertMessage(const String& targetName, Status status, uint16_t latency, bool isRecovery = false, unsigned long totalDowntime = 0);
  String formatTime(unsigned long seconds) const;
  String getCurrentTime() const;
  
  // HTTP communication
  bool sendMessage(const String& message);
  
  // Alert logic
  bool isTimeForAlert(int targetIndex, bool isRecovery = false) const;
  bool isHealthCheckHealthy(const String& response) const;
};
