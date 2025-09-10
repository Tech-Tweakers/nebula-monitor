#include "core/infrastructure/telegram_service/telegram_service.h"
#include "core/infrastructure/http_client/http_client.h"
#include "core/domain/alert/alert.h"
#include "core/infrastructure/ssl_mutex_manager/ssl_mutex_manager.h"
#include "core/infrastructure/memory_manager/memory_manager.h"
#include "core/infrastructure/ntp_service/ntp_service.h"
#include <ArduinoJson.h>

TelegramService::TelegramService() : enabled(false), sendingMessage(false) {
  // Initialize alert array
  for (int i = 0; i < 6; i++) {
    alerts[i] = nullptr;
    lastMessageIds[i] = 0;
    isThreadActive[i] = false;
  }
}

TelegramService::~TelegramService() {
  // Clean up alert objects
  for (int i = 0; i < 6; i++) {
    if (alerts[i]) {
      delete alerts[i];
      alerts[i] = nullptr;
    }
  }
}

bool TelegramService::initialize(const String& botToken, const String& chatId, bool enabled) {
  if (!enabled) {
    this->enabled = false;
    return true;
  }

  if (botToken.length() == 0 || chatId.length() == 0) {
    Serial.println("[TELEGRAM] ERROR: Bot token or chat ID not provided");
    return false;
  }

  this->botToken = botToken;
  this->chatId = chatId;
  this->enabled = true;

  // Initialize alert objects
  for (int i = 0; i < 6; i++) {
    if (!alerts[i]) {
      alerts[i] = new Alert(i, "Target " + String(i));
    }
  }

  Serial.println("[TELEGRAM] Service initialized successfully");
  return true;
}

void TelegramService::updateTargetStatus(int targetIndex, Status newStatus, uint16_t latency, const String& targetName) {
  if (!enabled || targetIndex < 0 || targetIndex >= 6 || !alerts[targetIndex]) {
    return;
  }

  Alert* alert = alerts[targetIndex];
  alert->updateStatus(newStatus, latency);

  if (alert->shouldSendAlert() && isTimeForAlert(targetIndex)) {
    sendAlert(targetIndex, targetName, newStatus, latency);
  } else if (alert->shouldSendRecovery() && isTimeForAlert(targetIndex, true)) {
    sendRecoveryAlert(targetIndex, targetName, latency);
  }
}

void TelegramService::sendAlert(int targetIndex, const String& targetName, Status status, uint16_t latency) {
  if (!enabled || sendingMessage) {
    return;
  }

  sendingMessage = true;
  // Get current downtime for DOWN alerts
  unsigned long currentDowntime = alerts[targetIndex] ? alerts[targetIndex]->getDowntime() : 0;
  String message = formatAlertMessage(targetName, status, latency, false, currentDowntime);
  
  // Send alert with reply thread support (isRecovery = false for down alerts)
  if (sendMessage(message, targetIndex, false)) {
    if (alerts[targetIndex]) {
      alerts[targetIndex]->markAlertSent();
    }
    Serial.printf("[TELEGRAM] Alert sent for target %d (%s) - Thread: %s\n", 
                  targetIndex, targetName.c_str(), 
                  isThreadActive[targetIndex] ? "Reply" : "New");
  } else {
    Serial.printf("[TELEGRAM] Failed to send alert for target %d (%s)\n", targetIndex, targetName.c_str());
  }
  
  sendingMessage = false;
}

void TelegramService::sendRecoveryAlert(int targetIndex, const String& targetName, uint16_t latency) {
  if (!enabled || sendingMessage) {
    return;
  }

  sendingMessage = true;
  
  // Get alert timing information
  Alert* alert = alerts[targetIndex];
  unsigned long totalDowntime = alert ? alert->getDowntime() : 0;
  unsigned long firstFailureTime = alert ? alert->getFirstFailureTime() : 0;
  unsigned long alertStartTime = alert ? alert->getAlertDowntimeStart() : 0;
  
  String message = formatRecoveryMessage(targetName, latency, totalDowntime, firstFailureTime, alertStartTime);
  
  // Send recovery with reply thread support (isRecovery = true ends the thread)
  if (sendMessage(message, targetIndex, true)) {
    if (alert) {
      alert->markRecovered();
      // Reset alert data for clean state - ready for next alert
      alert->reset();
    }
    Serial.printf("[TELEGRAM] Recovery alert sent for target %d (%s) - Thread ended\n", targetIndex, targetName.c_str());
  } else {
    Serial.printf("[TELEGRAM] Failed to send recovery alert for target %d (%s)\n", targetIndex, targetName.c_str());
  }
  
  sendingMessage = false;
}

void TelegramService::sendTestMessage(const String* targetNames, int targetCount) {
  if (!enabled) {
    Serial.println("[TELEGRAM] Service not active, skipping test message");
    return;
  }

  String testMessage = "🤖 <b>Nebula Monitor v2.5</b>\n";
  testMessage += "✅ <b>System Initialized Successfully!</b>\n\n";
  
  // WiFi Status
  if (WiFi.status() == WL_CONNECTED) {
    testMessage += "📶 <b>WiFi:</b> Connected\n";
    testMessage += "🌐 <b>IP:</b> " + WiFi.localIP().toString() + "\n";
    testMessage += "📡 <b>RSSI:</b> " + String(WiFi.RSSI()) + " dBm\n\n";
  } else {
    testMessage += "📶 <b>WiFi:</b> Disconnected\n\n";
  }
  
  // Services Status
  testMessage += "🔧 <b>Services:</b>\n";
  testMessage += "• Telegram: ✅ Active\n";
  testMessage += "• Display: ✅ Active\n";
  testMessage += "• Network Monitor: ✅ Active\n";
  testMessage += "• Task Manager: ✅ Active\n\n";
  
  // Targets Info
  testMessage += "🎯 <b>Monitoring Targets:</b>\n";
  if (targetNames && targetCount > 0) {
    for (int i = 0; i < targetCount; i++) {
      if (targetNames[i].length() > 0) {
        testMessage += "• " + targetNames[i] + "\n";
      }
    }
  } else {
    testMessage += "• No targets configured\n";
  }
  
  testMessage += "\n⏰ <b>Scan Interval:</b> 30s\n";
  testMessage += "🚨 <b>Alert Threshold:</b> 3 failures\n";
  testMessage += "⏱️ <b>Cooldown:</b> 5 minutes\n\n";
  testMessage += "🔄 <b>System is now monitoring...</b>";
  
  sendMessage(testMessage);
}

bool TelegramService::isActive() const {
  return enabled;
}

bool TelegramService::hasActiveAlerts() const {
  if (!enabled) return false;
  
  for (int i = 0; i < 6; i++) {
    if (alerts[i] && alerts[i]->isAlertActive()) {
      return true;
    }
  }
  return false;
}

int TelegramService::getFailureCount(int targetIndex) const {
  if (targetIndex < 0 || targetIndex >= 6 || !alerts[targetIndex]) {
    return 0;
  }
  return alerts[targetIndex]->getFailureCount();
}

String TelegramService::formatAlertMessage(const String& targetName, Status status, uint16_t latency, bool isRecovery, unsigned long totalDowntime) {
  String message = "";
  
  if (isRecovery) {
    message += "🟢 <b>SYSTEM ONLINE</b>\n\n";
    message += "🎉 <b>Target:</b> " + targetName + "\n\n";
    message += "⏱️ <b>Downtime:</b> " + formatTime(totalDowntime) + "\n";
    message += "📊 <b>Current Latency:</b> " + String(latency) + "ms\n\n";
    message += "✅ <b>Service is back online!</b>";
  } else if (status == DOWN) {
    message += "🚨 <b>SYSTEM DOWN!</b>\n\n";
    message += "🔴 <b>Target:</b> " + targetName + "\n\n";
    message += "🕐 <b>Detected:</b> " + NTPService::getCurrentDateTime() + "\n";
    message += "⏱️ <b>Downtime:</b> " + formatTime(totalDowntime) + "\n";
    message += "⚠️ <b>Status:</b> Unreachable\n\n";
    message += "🔍 <b>Waiting for recovery...</b>";
  } else {
    message += "❓ <b>UNKNOWN STATUS</b>\n\n";
    message += "🟡 <b>Target:</b> " + targetName + "\n";
    message += "📊 <b>Response:</b> " + String(latency) + "ms\n";
    message += "🕐 <b>Detected:</b> " + NTPService::getCurrentDateTime() + "\n\n";
    message += "🔍 <b>Status unclear, waiting...</b>";
  }
  
  return message;
}

String TelegramService::formatRecoveryMessage(const String& targetName, uint16_t latency, unsigned long totalDowntime, 
                                             unsigned long firstFailureTime, unsigned long alertStartTime) {
  String message = "";
  
  message += "🟢 <b>SYSTEM RECOVERED</b>\n\n";
  message += "🎉 <b>Target:</b> " + targetName + "\n\n";
  
  // Calculate times
  unsigned long currentTime = millis();
  unsigned long recoveryTime = currentTime;
  
  // Format failure start time (if available)
  String failureStartTime = "Unknown";
  if (firstFailureTime > 0) {
    // Convert millis to real time using NTP
    failureStartTime = convertMillisToRealTime(firstFailureTime);
  }
  
  // Format alert start time (if available)
  String alertStartTimeStr = "Unknown";
  if (alertStartTime > 0) {
    // Convert millis to real time using NTP
    alertStartTimeStr = convertMillisToRealTime(alertStartTime);
  }
  
  // Format recovery time
  String recoveryTimeStr = NTPService::getCurrentDateTime();
  
  message += "🕐 <b>First Failure:</b> " + failureStartTime + "\n";
  message += "✅ <b>Recovered At:</b> " + recoveryTimeStr + "\n\n";
  
  message += "⏱️ <b>Total Downtime:</b> " + formatTime(totalDowntime) + "\n";
  message += "📊 <b>Current Latency:</b> " + String(latency) + "ms\n";
  message += "🔄 <b>Status:</b> Online\n\n";
  
  message += "✅ <b>Service is fully operational!</b>";
  
  return message;
}

String TelegramService::formatTime(unsigned long seconds) const {
  if (seconds < 60) {
    return String(seconds) + "s";
  } else if (seconds < 3600) {
    unsigned long minutes = seconds / 60;
    unsigned long remainingSeconds = seconds % 60;
    if (remainingSeconds == 0) {
      return String(minutes) + "m";
    } else {
      return String(minutes) + "m " + String(remainingSeconds) + "s";
    }
  } else {
    unsigned long hours = seconds / 3600;
    unsigned long minutes = (seconds % 3600) / 60;
    if (minutes == 0) {
      return String(hours) + "h";
    } else {
      return String(hours) + "h " + String(minutes) + "m";
    }
  }
}

String TelegramService::getCurrentTime() const {
  // Use NTP service for real time instead of uptime
  String timeStr = NTPService::getCurrentDateTime();
  Serial.printf("[TELEGRAM] getCurrentTime: %s\n", timeStr.c_str());
  return timeStr;
}

String TelegramService::formatMillisToTime(unsigned long millisTime) const {
  // Convert millis to relative time from boot
  unsigned long seconds = millisTime / 1000;
  unsigned long hours = (seconds / 3600) % 24;
  unsigned long minutes = (seconds / 60) % 60;
  unsigned long secs = seconds % 60;
  
  String timeStr = "";
  if (hours < 10) timeStr += "0";
  timeStr += String(hours) + ":";
  if (minutes < 10) timeStr += "0";
  timeStr += String(minutes) + ":";
  if (secs < 10) timeStr += "0";
  timeStr += String(secs);
  
  return timeStr + " (uptime)";
}

String TelegramService::convertMillisToRealTime(unsigned long millisTime) const {
  // Try to get current NTP time
  String currentTimeStr = NTPService::getCurrentDateTime();
  
  // If NTP is working, calculate the real time
  if (!currentTimeStr.endsWith("(uptime)")) {
    // NTP is working, calculate the real time for the millis timestamp
    unsigned long currentMillis = millis();
    unsigned long timeDiff = currentMillis - millisTime;
    
    // Extract time part from DD/MM/YYYY HH:MM:SS format
    int spaceIndex = currentTimeStr.indexOf(' ');
    if (spaceIndex > 0) {
      String timePart = currentTimeStr.substring(spaceIndex + 1);
      
      // Parse HH:MM:SS
      int colon1 = timePart.indexOf(':');
      int colon2 = timePart.indexOf(':', colon1 + 1);
      
      if (colon1 > 0 && colon2 > 0) {
        int hours = timePart.substring(0, colon1).toInt();
        int minutes = timePart.substring(colon1 + 1, colon2).toInt();
        int seconds = timePart.substring(colon2 + 1).toInt();
        
        // Convert to total seconds since midnight
        unsigned long currentSeconds = hours * 3600 + minutes * 60 + seconds;
        
        // Subtract the time difference
        unsigned long targetSeconds = currentSeconds - (timeDiff / 1000);
        
        // Handle day rollover
        if (targetSeconds < 0) {
          targetSeconds += 86400; // Add 24 hours
        }
        
        // Convert back to HH:MM:SS
        int targetHours = (targetSeconds / 3600) % 24;
        int targetMinutes = (targetSeconds / 60) % 60;
        int targetSecs = targetSeconds % 60;
        
        String timeStr = "";
        if (targetHours < 10) timeStr += "0";
        timeStr += String(targetHours) + ":";
        if (targetMinutes < 10) timeStr += "0";
        timeStr += String(targetMinutes) + ":";
        if (targetSecs < 10) timeStr += "0";
        timeStr += String(targetSecs);
        
        return timeStr;
      }
    }
  }
  
  // Fallback to uptime format
  return formatMillisToTime(millisTime);
}

bool TelegramService::sendMessage(const String& message, int targetIndex, bool isRecovery) {
  if (!enabled) {
    return false;
  }

  // Use SSL mutex to prevent memory allocation conflicts
  SSLLock sslLock(3000); // 3 second timeout
  
  if (!sslLock.isLocked()) {
    Serial.println("[TELEGRAM] ERROR: Failed to acquire SSL lock!");
    return false;
  }

  // Check available memory before proceeding
  if (MemoryManager::getInstance().isMemoryLow()) {
    Serial.println("[TELEGRAM] WARNING: Low memory, skipping message");
    return false;
  }

  HTTPClient http;
  String url = "https://api.telegram.org/bot" + botToken + "/sendMessage";
  
  if (!http.begin(url)) {
    Serial.println("[TELEGRAM] ERROR: Failed to begin HTTP request");
    return false;
  }

  http.addHeader("Content-Type", "application/json");
  http.setTimeout(5000); // 5 second timeout
  
  // Create JSON payload with memory management
  DynamicJsonDocument doc(1024);
  doc["chat_id"] = chatId;
  doc["text"] = message;
  doc["parse_mode"] = "HTML";
  
  // Add reply_to_message_id if we have an active thread for this target
  if (targetIndex >= 0 && targetIndex < 6 && isThreadActive[targetIndex] && lastMessageIds[targetIndex] > 0) {
    doc["reply_to_message_id"] = lastMessageIds[targetIndex];
    Serial.printf("[TELEGRAM] Sending reply to message %d for target %d\n", lastMessageIds[targetIndex], targetIndex);
  }
  
  String payload;
  serializeJson(doc, payload);
  
  // Clear document to free memory
  doc.clear();
  
  Serial.printf("[TELEGRAM] Sending message (heap: %d bytes)\n", ESP.getFreeHeap());
  
  int httpResponseCode = http.POST(payload);
  http.end();
  
  // Force cleanup of payload
  payload = "";
  payload.reserve(0);
  
  if (httpResponseCode > 0) {
    if (httpResponseCode == 200) {
      Serial.println("[TELEGRAM] Message sent successfully");
      
      // TODO: Parse response to get real message_id from Telegram
      // Temporarily disabled to prevent hanging
      Serial.println("[TELEGRAM] Response parsing disabled to prevent hanging");
      uint32_t realMessageId = 0; // Will be implemented later
      
      // Update thread management for this target
      if (targetIndex >= 0 && targetIndex < 6) {
        if (isRecovery) {
          // Recovery ends the thread
          isThreadActive[targetIndex] = false;
          lastMessageIds[targetIndex] = 0;
          Serial.printf("[TELEGRAM] Thread ended for target %d (recovery)\n", targetIndex);
        } else {
          // Down alert continues or starts thread
          if (realMessageId > 0) {
            lastMessageIds[targetIndex] = realMessageId;
            isThreadActive[targetIndex] = true;
            Serial.printf("[TELEGRAM] Thread active for target %d (down) - message_id: %d\n", targetIndex, realMessageId);
          } else {
            Serial.printf("[TELEGRAM] WARNING: Could not get message_id for target %d\n", targetIndex);
          }
        }
      }
      
      return true;
    } else {
      Serial.printf("[TELEGRAM] HTTP error: %d\n", httpResponseCode);
    }
  } else {
    Serial.printf("[TELEGRAM] ERROR: HTTP request failed, code: %d\n", httpResponseCode);
  }
  
  return false;
}

bool TelegramService::isTimeForAlert(int targetIndex, bool isRecovery) const {
  if (targetIndex < 0 || targetIndex >= 6 || !alerts[targetIndex]) {
    return false;
  }

  unsigned long now = millis();
  unsigned long lastAlertTime = alerts[targetIndex]->getLastAlertTime();
  unsigned long cooldown = isRecovery ? ALERT_RECOVERY_COOLDOWN_MS : ALERT_COOLDOWN_MS;
  
  // If lastAlertTime is 0, it means no alert has been sent yet - allow immediate sending
  if (lastAlertTime == 0) {
    return true;
  }
  
  unsigned long timeSinceLastAlert = now - lastAlertTime;
  return timeSinceLastAlert >= cooldown;
}

bool TelegramService::isHealthCheckHealthy(const String& response) const {
  // Simple health check - look for common success indicators
  return response.indexOf("200") != -1 || 
         response.indexOf("OK") != -1 || 
         response.indexOf("success") != -1 ||
         response.indexOf("healthy") != -1;
}