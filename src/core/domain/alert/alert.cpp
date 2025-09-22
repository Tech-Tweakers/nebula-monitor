#include "core/domain/alert/alert.h"
#include "core/infrastructure/logger/logger.h"

Alert::Alert(int index, const String& name) 
  : targetIndex(index), targetName(name ? name : "Unknown"), 
    currentStatus(UNKNOWN), lastStatus(UNKNOWN), failureCount(0),
    firstFailureTime(0), lastAlertTime(0), isActive(false), 
    alertSent(false), lastLatency(0), alertDowntimeStart(0), totalDowntime(0) {
}

void Alert::updateStatus(Status newStatus, uint16_t latency) {
  lastLatency = latency;
  
  if (currentStatus != newStatus) {
    lastStatus = currentStatus;
    currentStatus = newStatus;
    
    if (newStatus == DOWN || newStatus == UNKNOWN) {
      // Target went down or became unknown (both are failures)
      failureCount++;
      if (failureCount == 1) {
        firstFailureTime = millis();
        Serial_printf("[ALERT] %s: Downtime started (status: %s)\n", targetName.c_str(), 
                     newStatus == DOWN ? "DOWN" : "UNKNOWN");
      }
      Serial_printf("[ALERT] %s: Failure #%d (status: %s)\n", targetName.c_str(), failureCount,
                   newStatus == DOWN ? "DOWN" : "UNKNOWN");
    } else if (newStatus == UP && (lastStatus == DOWN || lastStatus == UNKNOWN)) {
      // Recovery detected
      Serial_printf("[ALERT] %s: Recovery detected\n", targetName.c_str());
      if (alertSent) {
        // Don't mark as recovered yet - let the recovery logic handle it
      } else {
        // Reset downtime tracking even if no alert was sent
        firstFailureTime = 0;
        failureCount = 0;
        Serial_printf("[ALERT] %s: Quick recovery - resetting downtime tracking\n", targetName.c_str());
      }
    }
  } else if (newStatus == DOWN || newStatus == UNKNOWN) {
    // Continuous failure (DOWN or UNKNOWN)
    failureCount++;
    Serial_printf("[ALERT] %s: Continuous failure #%d (status: %s)\n", targetName.c_str(), failureCount,
                 newStatus == DOWN ? "DOWN" : "UNKNOWN");
  }
}

bool Alert::shouldSendAlert() const {
  // CRITICAL FIX: Alert on both DOWN and UNKNOWN status
  if (currentStatus != DOWN && currentStatus != UNKNOWN) return false;
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
  Serial_printf("[ALERT] %s: Alert marked as sent\n", targetName.c_str());
}

void Alert::markRecovered() {
  // Save total downtime before resetting
  if (firstFailureTime > 0) {
    totalDowntime = (millis() - firstFailureTime) / 1000;
  }
  
  failureCount = 0;
  alertSent = false;
  firstFailureTime = 0;
  alertDowntimeStart = 0;
  isActive = false;
  lastAlertTime = millis();
  Serial_printf("[ALERT] %s: Marked as recovered (total downtime: %lu seconds)\n", targetName.c_str(), totalDowntime);
}

unsigned long Alert::getDowntime() const {
  if (firstFailureTime > 0) {
    // Target is still down, return current downtime
    return (millis() - firstFailureTime) / 1000;
  } else if (totalDowntime > 0) {
    // Target recovered, return saved total downtime
    return totalDowntime;
  }
  return 0;
}

void Alert::setTargetName(const String& name) {
  if (name && name.length() > 0) {
    targetName = String(name);
  }
}

void Alert::reset() {
  // Reset all alert data for clean state
  failureCount = 0;
  firstFailureTime = 0;
  lastAlertTime = 0;
  isActive = false;
  alertSent = false;
  lastLatency = 0;
  alertDowntimeStart = 0;
  totalDowntime = 0;
  currentStatus = UNKNOWN;
  lastStatus = UNKNOWN;
  
  Serial_printf("[ALERT] %s: Reset complete - clean state for new alerts\n", targetName.c_str());
}

void Alert::printState() const {
  Serial_printf("[ALERT] %s: status=%d, failures=%d, active=%s, sent=%s\n",
               targetName.c_str(), currentStatus, failureCount,
               isActive ? "true" : "false", alertSent ? "true" : "false");
}
