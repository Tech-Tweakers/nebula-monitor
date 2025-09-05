#include "alert.h"

Alert::Alert(int index, const String& name) 
  : targetIndex(index), targetName(name ? name : "Unknown"), 
    currentStatus(UNKNOWN), lastStatus(UNKNOWN), failureCount(0),
    firstFailureTime(0), lastAlertTime(0), isActive(false), 
    alertSent(false), lastLatency(0), alertDowntimeStart(0) {
}

void Alert::updateStatus(Status newStatus, uint16_t latency) {
  lastLatency = latency;
  
  if (currentStatus != newStatus) {
    lastStatus = currentStatus;
    currentStatus = newStatus;
    
    if (newStatus == DOWN) {
      // Target went down
      failureCount++;
      if (failureCount == 1) {
        firstFailureTime = millis();
        Serial.printf("[ALERT] %s: Downtime started\n", targetName.c_str());
      }
      Serial.printf("[ALERT] %s: Failure #%d\n", targetName.c_str(), failureCount);
    } else if (newStatus == UP && lastStatus == DOWN) {
      // Recovery detected
      Serial.printf("[ALERT] %s: Recovery detected\n", targetName.c_str());
      if (alertSent) {
        // Don't mark as recovered yet - let the recovery logic handle it
      } else {
        // Reset downtime tracking even if no alert was sent
        firstFailureTime = 0;
        failureCount = 0;
        Serial.printf("[ALERT] %s: Quick recovery - resetting downtime tracking\n", targetName.c_str());
      }
    }
  } else if (newStatus == DOWN) {
    // Continuous failure
    failureCount++;
    Serial.printf("[ALERT] %s: Continuous failure #%d\n", targetName.c_str(), failureCount);
  }
}

bool Alert::shouldSendAlert() const {
  if (currentStatus != DOWN) return false;
  if (failureCount < MAX_FAILURES_BEFORE_ALERT) return false;
  
  // Check cooldown - allow re-sending after cooldown period
  unsigned long now = millis();
  if (lastAlertTime > 0 && (now - lastAlertTime) < ALERT_COOLDOWN_MS) {
    return false;
  }
  
  return true;
}

bool Alert::shouldSendRecovery() const {
  if (currentStatus != UP) return false;
  if (!alertSent) return false;
  if (alertDowntimeStart == 0) return false;
  
  // Check recovery cooldown
  unsigned long now = millis();
  if (lastAlertTime > 0 && (now - lastAlertTime) < ALERT_RECOVERY_COOLDOWN_MS) {
    return false;
  }
  
  return true;
}

void Alert::markAlertSent() {
  alertSent = true;
  lastAlertTime = millis();
  alertDowntimeStart = firstFailureTime > 0 ? firstFailureTime : millis();
  isActive = true;
  Serial.printf("[ALERT] %s: Alert marked as sent\n", targetName.c_str());
}

void Alert::markRecovered() {
  failureCount = 0;
  alertSent = false;
  firstFailureTime = 0;
  alertDowntimeStart = 0;
  isActive = false;
  lastAlertTime = millis();
  Serial.printf("[ALERT] %s: Marked as recovered\n", targetName.c_str());
}

unsigned long Alert::getDowntime() const {
  return firstFailureTime > 0 ? (millis() - firstFailureTime) : 0;
}

void Alert::setTargetName(const String& name) {
  if (name && name.length() > 0) {
    targetName = String(name);
  }
}

void Alert::printState() const {
  Serial.printf("[ALERT] %s: status=%d, failures=%d, active=%s, sent=%s\n",
               targetName.c_str(), currentStatus, failureCount,
               isActive ? "true" : "false", alertSent ? "true" : "false");
}
