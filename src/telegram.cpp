#include "telegram.hpp"
#include "net.hpp"
#include <HTTPClient.h>

// Inicialização dos membros estáticos
bool TelegramAlerts::isEnabled = false;
String TelegramAlerts::botToken = "";
String TelegramAlerts::chatId = "";
AlertState TelegramAlerts::alertStates[6];
bool TelegramAlerts::sendingMessage = false;

bool TelegramAlerts::begin() {
  Serial.println("[TELEGRAM] Initializing alerts system...");
  
  // Verificar se está habilitado
  if (!TELEGRAM_ENABLED) {
    Serial.println("[TELEGRAM] Alerts disabled in configuration");
    return false;
  }
  
  // Verificar se token está configurado
  DEBUG_LOGF("[TELEGRAM] Verificando token: '%s' (length=%d)\n", TELEGRAM_BOT_TOKEN, strlen(TELEGRAM_BOT_TOKEN));
  if (strlen(TELEGRAM_BOT_TOKEN) == 0 || strcmp(TELEGRAM_BOT_TOKEN, "YOUR_BOT_TOKEN_HERE") == 0) {
    Serial.println("[TELEGRAM] Bot token not configured!");
    return false;
  }
  
  // Verificar se chat ID está configurado
  if (strlen(TELEGRAM_CHAT_ID) == 0 || strcmp(TELEGRAM_CHAT_ID, "YOUR_CHAT_ID_HERE") == 0) {
    Serial.println("[TELEGRAM] Chat ID not configured!");
    return false;
  }
  
  botToken = String(TELEGRAM_BOT_TOKEN);
  chatId = String(TELEGRAM_CHAT_ID);
  isEnabled = true;
  
  // Inicializar estados de alerta
  for (int i = 0; i < 6; i++) {
    alertStates[i].failure_count = 0;
    alertStates[i].last_status = UNKNOWN;
    alertStates[i].last_alert = 0;
    alertStates[i].alert_sent = false;
    alertStates[i].downtime_start = 0;
    alertStates[i].alert_downtime_start = 0;
  }
  
  Serial.println("[TELEGRAM] Alerts system initialized successfully!");
  Serial.printf("[TELEGRAM] Bot: %s\n", botToken.substring(0, 10).c_str());
  Serial.printf("[TELEGRAM] Chat: %s\n", chatId.c_str());
  Serial.printf("[TELEGRAM] MAX_FAILURES_BEFORE_ALERT: %d\n", MAX_FAILURES_BEFORE_ALERT);
  Serial.printf("[TELEGRAM] ALERT_COOLDOWN_MS: %d\n", ALERT_COOLDOWN_MS);
  
  return true;
}

// Funções removidas: end(), setBotToken(), setChatId(), enable() - não utilizadas

void TelegramAlerts::updateTargetStatus(int targetIndex, Status newStatus, uint16_t latency) {
  if (!isActive() || targetIndex < 0 || targetIndex >= 6) {
    Serial.printf("[TELEGRAM] updateTargetStatus: not active or invalid index (targetIndex=%d, isActive=%s)\n", 
                 targetIndex, isActive() ? "true" : "false");
    return;
  }
  
  AlertState& state = alertStates[targetIndex];
  unsigned long now = millis();
  
  DEBUG_LOGF("[TELEGRAM] updateTargetStatus: targetIndex=%d, newStatus=%d, latency=%d, lastStatus=%d, failureCount=%d\n", 
               targetIndex, newStatus, latency, state.last_status, state.failure_count);
  
  // Verificar mudança de status
  if (state.last_status != newStatus) {
    DEBUG_LOGF("[TELEGRAM] Status mudou de %d para %d\n", state.last_status, newStatus);
    
    if (newStatus == DOWN) {
      // Status mudou para DOWN
      state.failure_count++;
      
      // Marcar início do downtime se é a primeira falha
      if (state.failure_count == 1) {
        state.downtime_start = now;
        Serial.printf("[TELEGRAM] Target %d: Downtime started\n", targetIndex);
      }
      
      Serial.printf("[TELEGRAM] Target %d: Failure #%d\n", targetIndex, state.failure_count);
      
      // Enviar alerta se atingiu o limite
      if (state.failure_count >= MAX_FAILURES_BEFORE_ALERT && isTimeForAlert(targetIndex)) {
        Serial.printf("[TELEGRAM] Sending alert for target %d (failures: %d)\n", targetIndex, state.failure_count);
        // Marcar início do downtime de alerta ANTES de enviar a mensagem (para o DOWN incluir duração)
        if (state.alert_downtime_start == 0) {
          state.alert_downtime_start = state.downtime_start > 0 ? state.downtime_start : now;
          Serial.printf("[TELEGRAM] Target %d: alert_downtime_start=%lu (pre-ALERT)\n", targetIndex, state.alert_downtime_start);
        }
        sendAlert(targetIndex, "Target", newStatus, latency);
        state.last_alert = now;
        state.alert_sent = true;
      } else {
        Serial.printf("[TELEGRAM] Not time to alert yet (failures: %d, limit: %d)\n", 
                     state.failure_count, MAX_FAILURES_BEFORE_ALERT);
      }
    } else if (newStatus == UP && state.last_status == DOWN) {
      // Recuperação: status mudou de DOWN para UP
      Serial.printf("[TELEGRAM] Recovery detected for target %d\n", targetIndex);
      bool hadOutage = state.downtime_start > 0;
      bool canRecover = isTimeForAlert(targetIndex, true);
      Serial.printf("[TELEGRAM] Recovery check: alert_sent=%s, hadOutage=%s, canRecover=%s, last_alert=%lu, downtime_start=%lu\n",
                    state.alert_sent ? "true" : "false",
                    hadOutage ? "true" : "false",
                    canRecover ? "true" : "false",
                    state.last_alert,
                    state.downtime_start);
      // Enviar recuperação SOMENTE se houve alerta DOWN prévio (usa alert_downtime_start)
      if (state.alert_sent && state.alert_downtime_start > 0 && canRecover) {
        sendRecoveryAlert(targetIndex, "Target", latency);
        state.last_alert = now;
        // Reset somente após enviar recovery
        state.failure_count = 0;
        state.alert_sent = false;
        state.downtime_start = 0;  // Reset downtime tracking
        state.alert_downtime_start = 0; // Reset downtime tracking do alerta
      } else {
        Serial.println("[TELEGRAM] Recovery NOT sent (waiting for cooldown or no downtime recorded)");
      }
    }
    // Atualizar last_status após tratar a mudança
    state.last_status = newStatus;
  // Se status permaneceu UP, verificar se há recovery pendente (por cooldown) e tentar enviar
  } else if (newStatus == UP) {
    if (state.alert_sent && state.alert_downtime_start > 0 && isTimeForAlert(targetIndex, true)) {
      Serial.printf("[TELEGRAM] Pending recovery: sending now (target %d)\n", targetIndex);
      sendRecoveryAlert(targetIndex, "Target", latency);
      state.last_alert = now;
      state.failure_count = 0;
      state.alert_sent = false;
      state.downtime_start = 0;
      state.alert_downtime_start = 0;
    }
    
    state.last_status = newStatus;
  } else if (newStatus == DOWN) {
    // Status continua DOWN, incrementar contador
    state.failure_count++;
    Serial.printf("[TELEGRAM] Target %d: Continuous failure #%d\n", targetIndex, state.failure_count);
    
      // Verificar se deve enviar alerta
  Serial.printf("[TELEGRAM] Checking alert: failures=%d, limit=%d, isTimeForAlert=%s\n", 
               state.failure_count, MAX_FAILURES_BEFORE_ALERT, isTimeForAlert(targetIndex) ? "true" : "false");
  
  if (state.failure_count >= MAX_FAILURES_BEFORE_ALERT && isTimeForAlert(targetIndex)) {
    // Garantir início do downtime de alerta também no caminho contínuo
    if (state.alert_downtime_start == 0) {
      state.alert_downtime_start = state.downtime_start > 0 ? state.downtime_start : now;
      Serial.printf("[TELEGRAM] Target %d: alert_downtime_start=%lu (continuous)\n", targetIndex, state.alert_downtime_start);
    }
    Serial.printf("[TELEGRAM] Sending alert for target %d (continuous failures: %d)\n", targetIndex, state.failure_count);
    sendAlert(targetIndex, "Target", newStatus, latency);
    state.last_alert = now;
    state.alert_sent = true;
  } else {
    Serial.printf("[TELEGRAM] Alert NOT sent: failures=%d, limit=%d, isTimeForAlert=%s\n", 
                 state.failure_count, MAX_FAILURES_BEFORE_ALERT, isTimeForAlert(targetIndex) ? "true" : "false");
  }
  }
}

void TelegramAlerts::sendAlert(int targetIndex, const char* targetName, Status status, uint16_t latency) {
  if (!isActive()) return;
  
  // Usar o nome real do target
  const char* realTargetName = "Target";
  if (targetIndex >= 0 && targetIndex < 6) {
    // Usar os nomes dos targets do main.cpp
    const char* targetNames[] = {
      "Proxmox HV", "Router #1", "Router #2", 
      "Polaris API", "Polaris INT", "Polaris WEB"
    };
    realTargetName = targetNames[targetIndex];
  }
  
  // Calcular downtime atual (em segundos) baseado em alert_downtime_start se disponível
  unsigned long currentDowntime = 0;
  if (targetIndex >= 0 && targetIndex < 6 && alertStates[targetIndex].alert_downtime_start > 0) {
    currentDowntime = (millis() - alertStates[targetIndex].alert_downtime_start) / 1000;
  }

  String message = formatAlertMessage(realTargetName, status, latency, false, currentDowntime);
  Serial.printf("[TELEGRAM] Sending alert for target %d: %s\n", targetIndex, realTargetName);
  
  if (sendMessage(message)) {
    Serial.println("[TELEGRAM] Alert sent successfully!");
  } else {
    Serial.println("[TELEGRAM] Failed to send alert!");
  }
}

void TelegramAlerts::sendRecoveryAlert(int targetIndex, const char* targetName, uint16_t latency) {
  if (!isActive()) return;
  
  // Usar o nome real do target
  const char* realTargetName = "Target";
  if (targetIndex >= 0 && targetIndex < 6) {
    // Usar os nomes dos targets do main.cpp
    const char* targetNames[] = {
      "Proxmox HV", "Router #1", "Router #2", 
      "Polaris API", "Polaris INT", "Polaris WEB"
    };
    realTargetName = targetNames[targetIndex];
  }
  
  // Calcular tempo total de downtime com base em alert_downtime_start
  unsigned long totalDowntime = 0;
  if (targetIndex >= 0 && targetIndex < 6 && alertStates[targetIndex].alert_downtime_start > 0) {
    totalDowntime = (millis() - alertStates[targetIndex].alert_downtime_start) / 1000;
  }
  
  String message = formatAlertMessage(realTargetName, UP, latency, true, totalDowntime);
  Serial.printf("[TELEGRAM] Sending recovery alert for target %d: %s (downtime: %lus)\n", 
               targetIndex, realTargetName, totalDowntime);
  
  if (sendMessage(message)) {
    Serial.println("[TELEGRAM] Recovery alert sent successfully!");
  } else {
    Serial.println("[TELEGRAM] Failed to send recovery alert!");
  }
}

String TelegramAlerts::formatAlertMessage(const char* targetName, Status status, uint16_t latency, bool isRecovery, unsigned long totalDowntime) {
  String message = "";
  
  // Função para formatar tempo em HH:MM:SS
  auto formatTime = [](unsigned long seconds) -> String {
    unsigned long hours = seconds / 3600;
    unsigned long minutes = (seconds % 3600) / 60;
    unsigned long secs = seconds % 60;
    
    String result = "";
    if (hours > 0) {
      result += String(hours) + ":";
      if (minutes < 10) result += "0";
    }
    result += String(minutes) + ":";
    if (secs < 10) result += "0";
    result += String(secs);
    
    return result;
  };
  
  if (isRecovery) {
    message = "🟢 *ONLINE* - " + String(targetName) + "\n\n";
    message += "✅ *" + String(targetName) + "* está funcionando novamente!\n";
    message += "⏱️ Latência: " + String(latency) + " ms\n";
    if (totalDowntime > 0) {
      message += "⏰ Downtime total: " + formatTime(totalDowntime) + "\n";
    }
  } else {
    message = "🔴 *ALERTA* - " + String(targetName) + "\n\n";
    message += "❌ *" + String(targetName) + "* está com problemas!\n";
    message += "📊 Status: " + String(status == DOWN ? "DOWN" : "UNKNOWN") + "\n";
    if (latency > 0) {
      message += "⏱️ Última latência: " + String(latency) + " ms\n";
    }
    if (totalDowntime > 0) {
      message += "🕐 " + formatTime(totalDowntime) + " de downtime\n";
    }
  }
  
  message += "\n🌌 _Nebula Monitor v2.3_";
  
  return message;
}

bool TelegramAlerts::sendMessage(const String& message) {
  if (!isActive() || WiFi.status() != WL_CONNECTED) {
    Serial.println("[TELEGRAM] Cannot send message - WiFi disconnected");
    return false;
  }
  
  sendingMessage = true; // Set flag to indicate sending message
  
  String url = "https://api.telegram.org/bot" + botToken + "/sendMessage";
  
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  // Criar JSON da mensagem manualmente (mais simples)
  String jsonString = "{";
  jsonString += "\"chat_id\":\"" + chatId + "\",";
  jsonString += "\"text\":\"" + message + "\",";
  jsonString += "\"parse_mode\":\"Markdown\",";
  jsonString += "\"disable_web_page_preview\":true";
  jsonString += "}";
  
  Serial.printf("[TELEGRAM] Sending to: %s\n", url.c_str());
  Serial.printf("[TELEGRAM] Message: %s\n", message.c_str());
  
  int httpResponseCode = http.POST(jsonString);
  
  bool success = false;
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.printf("[TELEGRAM] HTTP Response: %d\n", httpResponseCode);
    Serial.printf("[TELEGRAM] Response: %s\n", response.c_str());
    
    http.end();
    success = httpResponseCode == 200;
  } else {
    Serial.printf("[TELEGRAM] HTTP Error: %d\n", httpResponseCode);
    http.end();
    success = false;
  }
  
  sendingMessage = false; // Reset flag after sending
  return success;
}

// Nova função para verificar se o payload JSON indica status healthy
bool TelegramAlerts::isHealthCheckHealthy(const String& response) {
  // Verificar se contém "status":"healthy"
  if (response.indexOf("\"status\":\"healthy\"") > 0) {
    Serial.println("[HEALTH] Healthy status found in payload");
    return true;
  }
  
  // Verificar se contém "status":"unhealthy" ou "status":"down"
  if (response.indexOf("\"status\":\"unhealthy\"") > 0 || 
      response.indexOf("\"status\":\"down\"") > 0) {
    Serial.println("[HEALTH] Unhealthy status found in payload");
    return false;
  }
  
  // Se não encontrou status específico, considerar como não healthy
  Serial.println("[HEALTH] Status not found in payload, considering as not healthy");
  return false;
}

bool TelegramAlerts::isTimeForAlert(int targetIndex, bool isRecovery) {
  if (targetIndex < 0 || targetIndex >= 6) return false;
  
  AlertState& state = alertStates[targetIndex];
  unsigned long now = millis();
  unsigned long cooldown = isRecovery ? ALERT_RECOVERY_COOLDOWN_MS : ALERT_COOLDOWN_MS;
  
  // Evitar enviar recovery como primeiro alerta (ex.: logo após o boot)
  if (isRecovery && state.last_alert == 0) {
    Serial.printf("[TELEGRAM] isTimeForAlert: targetIndex=%d, RECOVERY BLOCKED (first alert)\n", targetIndex);
    return false;
  }
  // Se nunca enviou alerta (last_alert = 0), pode enviar (somente para ALERT)
  if (state.last_alert == 0) {
    Serial.printf("[TELEGRAM] isTimeForAlert: targetIndex=%d, FIRST ALERT (last_alert=0)\n", targetIndex);
    return true;
  }

  // Se for recovery e cooldown configurado para 0, permitir envio imediato
  if (isRecovery && cooldown == 0) {
    Serial.printf("[TELEGRAM] isTimeForAlert: targetIndex=%d, IMMEDIATE RECOVERY (cooldown=0)\n", targetIndex);
    return true;
  }
  
  unsigned long timeSinceLastAlert = now - state.last_alert;
  
  Serial.printf("[TELEGRAM] isTimeForAlert: targetIndex=%d, isRecovery=%s, last_alert=%lu, now=%lu, timeSince=%lu, cooldown=%lu\n", 
               targetIndex, isRecovery ? "true" : "false", state.last_alert, now, timeSinceLastAlert, cooldown);
  
  bool canAlert = (timeSinceLastAlert) >= cooldown;
  Serial.printf("[TELEGRAM] isTimeForAlert result: %s\n", canAlert ? "true" : "false");
  
  return canAlert;
}

int TelegramAlerts::getFailureCount(int targetIndex) {
  if (targetIndex < 0 || targetIndex >= 6) return 0;
  return alertStates[targetIndex].failure_count;
}

// Função removida: resetFailureCount() - não utilizada

bool TelegramAlerts::hasActiveAlerts() {
  if (!isActive()) return false;
  
  // Verificar se há pelo menos um target com alerta ativo
  for (int i = 0; i < 6; i++) {
    if (alertStates[i].alert_sent) {
      return true; // Há pelo menos um alerta ativo
    }
  }
  return false; // Nenhum alerta ativo
}

// Funções de conveniência
bool initTelegramAlerts() {
  return TelegramAlerts::begin();
}

void updateTelegramAlert(int targetIndex, Status status, uint16_t latency) {
  TelegramAlerts::updateTargetStatus(targetIndex, status, latency);
}

void sendTestTelegramAlert() {
  if (TelegramAlerts::isActive()) {
    String message = "🚀 *Nebula Monitor v2.3*\n\n";
    message += "📊 *Monitorando:* " + String(6) + " targets\n";
    message += "🔔 *Threshold:* " + String(MAX_FAILURES_BEFORE_ALERT) + " falhas\n";
    message += "⏰ *Cooldown:* " + String(ALERT_COOLDOWN_MS / 1000) + "s\n\n";
    message += "_Tech Tweakers - 2025_";
    
    TelegramAlerts::sendMessage(message);
  }
}




