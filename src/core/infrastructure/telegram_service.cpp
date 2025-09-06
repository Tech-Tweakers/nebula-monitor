#include "telegram_service.h"
#include "http_client.h"
#include "../domain/alert.h"
#include <ArduinoJson.h>

TelegramService::TelegramService() : enabled(false), sendingMessage(false) {
  // Initialize alert array
  for (int i = 0; i < 6; i++) {
    alerts[i] = nullptr;
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
  String message = formatAlertMessage(targetName, status, latency);
  
  if (sendMessage(message)) {
    if (alerts[targetIndex]) {
      alerts[targetIndex]->markAlertSent();
    }
    Serial.printf("[TELEGRAM] Alert sent for target %d (%s)\n", targetIndex, targetName.c_str());
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
  String message = formatAlertMessage(targetName, UP, latency, true, 
                                    alerts[targetIndex] ? alerts[targetIndex]->getDowntime() : 0);
  
  if (sendMessage(message)) {
    if (alerts[targetIndex]) {
      alerts[targetIndex]->markRecovered();
    }
    Serial.printf("[TELEGRAM] Recovery alert sent for target %d (%s)\n", targetIndex, targetName.c_str());
  } else {
    Serial.printf("[TELEGRAM] Failed to send recovery alert for target %d (%s)\n", targetIndex, targetName.c_str());
  }
  
  sendingMessage = false;
}

void TelegramService::sendTestMessage() {
  if (!enabled) {
    Serial.println("[TELEGRAM] Service not active, skipping test message");
    return;
  }

  String testMessage = "🤖 <b>Nebula Monitor v2.4</b>\n";
  testMessage += "✅ Telegram service is working!\n";
  testMessage += "🕐 Test time: " + String(millis()) + "ms";
  
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
  String message = "🚨 <b>Network Alert</b>\n\n";
  
  if (isRecovery) {
    message += "🟢 <b>RECOVERED:</b> " + targetName + "\n";
    message += "⏱️ <b>Downtime:</b> " + formatTime(totalDowntime) + "\n";
    message += "🕐 <b>Recovered:</b> " + String(millis() / 1000) + "s ago";
  } else if (status == DOWN) {
    message += "🔴 <b>DOWN:</b> " + targetName + "\n";
    message += "⏰ <b>Latency:</b> " + String(latency) + "ms\n";
    message += "🕐 <b>Detected:</b> " + String(millis() / 1000) + "s ago";
  } else {
    message += "🟡 <b>UNKNOWN:</b> " + targetName + "\n";
    message += "⏰ <b>Latency:</b> " + String(latency) + "ms\n";
    message += "🕐 <b>Detected:</b> " + String(millis() / 1000) + "s ago";
  }
  
  return message;
}

String TelegramService::formatTime(unsigned long seconds) const {
  if (seconds < 60) {
    return String(seconds) + "s";
  } else if (seconds < 3600) {
    return String(seconds / 60) + "m " + String(seconds % 60) + "s";
  } else {
    unsigned long hours = seconds / 3600;
    unsigned long minutes = (seconds % 3600) / 60;
    return String(hours) + "h " + String(minutes) + "m";
  }
}

bool TelegramService::sendMessage(const String& message) {
  if (!enabled) {
    return false;
  }

  HTTPClient http;
  String url = "https://api.telegram.org/bot" + botToken + "/sendMessage";
  
  if (!http.begin(url)) {
    Serial.println("[TELEGRAM] ERROR: Failed to begin HTTP request");
    return false;
  }

  http.addHeader("Content-Type", "application/json");
  
  // Create JSON payload
  DynamicJsonDocument doc(1024);
  doc["chat_id"] = chatId;
  doc["text"] = message;
  doc["parse_mode"] = "HTML";
  
  String payload;
  serializeJson(doc, payload);
  
  int httpResponseCode = http.POST(payload);
  http.end();
  
  if (httpResponseCode > 0) {
    if (httpResponseCode == 200) {
      return true;
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
  unsigned long cooldown = isRecovery ? ALERT_RECOVERY_COOLDOWN_MS : ALERT_COOLDOWN_MS;
  
  return (now - alerts[targetIndex]->getLastAlertTime()) >= cooldown;
}

bool TelegramService::isHealthCheckHealthy(const String& response) const {
  // Simple health check - look for common success indicators
  return response.indexOf("200") != -1 || 
         response.indexOf("OK") != -1 || 
         response.indexOf("success") != -1 ||
         response.indexOf("healthy") != -1;
}