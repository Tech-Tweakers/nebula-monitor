#pragma once
#include "core/domain/status.h"
#include <Arduino.h>

class Alert {
private:
  int targetIndex;
  String targetName;
  Status currentStatus;
  Status lastStatus;
  uint8_t failureCount;
  unsigned long firstFailureTime;
  unsigned long lastAlertTime;
  bool isActive;
  bool alertSent;
  uint16_t lastLatency;
  unsigned long alertDowntimeStart;
  unsigned long totalDowntime;
  
  // Configuration constants
  static const uint8_t MAX_FAILURES_BEFORE_ALERT = 3;
  static const unsigned long ALERT_COOLDOWN_MS = 300000; // 5 minutes
  static const unsigned long ALERT_RECOVERY_COOLDOWN_MS = 60000; // 1 minute
  
public:
  // Constructor
  Alert(int index, const String& name);
  
  // Status management
  void updateStatus(Status newStatus, uint16_t latency);
  bool shouldSendAlert() const;
  bool shouldSendRecovery() const;
  void markAlertSent();
  void markRecovered();
  
  // Getters
  bool isAlertActive() const { return isActive; }
  String getTargetName() const { return targetName; }
  unsigned long getDowntime() const;
  uint8_t getFailureCount() const { return failureCount; }
  Status getCurrentStatus() const { return currentStatus; }
  Status getLastStatus() const { return lastStatus; }
  bool hasAlertBeenSent() const { return alertSent; }
  unsigned long getLastAlertTime() const { return lastAlertTime; }
  uint16_t getLastLatency() const { return lastLatency; }
  unsigned long getFirstFailureTime() const { return firstFailureTime; }
  unsigned long getAlertDowntimeStart() const { return alertDowntimeStart; }
  
  // Setters
  void setTargetName(const String& name);
  
  // Debug
  void printState() const;
};
