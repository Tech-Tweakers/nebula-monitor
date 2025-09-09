#include "telegram_service.h"
#include "http_client.h"
#include "../domain/alert.h"
#include "ssl_mutex_manager.h"
#include "memory_manager.h"
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

void TelegramService::sendTestMessage(const String* targetNames, int targetCount) {
  if (!enabled) {
    Serial.println("[TELEGRAM] Service not active, skipping test message");
    return;
  }

  String testMessage = "🤖 <b>Nebula Monitor v2.4</b>\n";
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
    message += "🎉 <b>SYSTEM ONLINE</b>\n\n";
    message += "🟢 <b>Target:</b> " + targetName + "\n";
    message += "⏱️ <b>Downtime:</b> " + formatTime(totalDowntime) + "\n";
    message += "📊 <b>Current Latency:</b> " + String(latency) + "ms\n\n";
    message += "✅ <b>Service is back online!</b>";
  } else if (status == DOWN) {
    message += "🚨 <b>SYSTEM DOWN</b>\n\n";
    message += "🔴 <b>Target:</b> " + targetName + "\n";
    message += "📊 <b>Last Response:</b> " + String(latency) + "ms\n";
    message += "🕐 <b>Detected:</b> " + getCurrentTime() + "\n";
    message += "⚠️ <b>Status:</b> Unreachable\n\n";
    message += "🔍 <b>Waiting for recovery...</b>";
  } else {
    message += "❓ <b>UNKNOWN STATUS</b>\n\n";
    message += "🟡 <b>Target:</b> " + targetName + "\n";
    message += "📊 <b>Response:</b> " + String(latency) + "ms\n";
    message += "🕐 <b>Detected:</b> " + getCurrentTime() + "\n\n";
    message += "🔍 <b>Status unclear, waiting...</b>";
  }
  
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
  return NTPService::getCurrentTime();
}

bool TelegramService::sendMessage(const String& message) {
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
  http.setTimeout(2000); // 2 second timeout
  
  // Create JSON payload with memory management
  DynamicJsonDocument doc(1024);
  doc["chat_id"] = chatId;
  doc["text"] = message;
  doc["parse_mode"] = "HTML";
  
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