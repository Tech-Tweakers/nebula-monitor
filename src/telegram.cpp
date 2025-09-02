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
  Serial.println("[TELEGRAM] Inicializando sistema de alertas...");
  
  // Verificar se está habilitado
  if (!TELEGRAM_ENABLED) {
    Serial.println("[TELEGRAM] Alertas desabilitados na configuração");
    return false;
  }
  
  // Verificar se token está configurado
  DEBUG_LOGF("[TELEGRAM] Verificando token: '%s' (length=%d)\n", TELEGRAM_BOT_TOKEN, strlen(TELEGRAM_BOT_TOKEN));
  if (strlen(TELEGRAM_BOT_TOKEN) == 0 || strcmp(TELEGRAM_BOT_TOKEN, "YOUR_BOT_TOKEN_HERE") == 0) {
    Serial.println("[TELEGRAM] Token do bot não configurado!");
    return false;
  }
  
  // Verificar se chat ID está configurado
  if (strlen(TELEGRAM_CHAT_ID) == 0 || strcmp(TELEGRAM_CHAT_ID, "YOUR_CHAT_ID_HERE") == 0) {
    Serial.println("[TELEGRAM] Chat ID não configurado!");
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
  }
  
  Serial.println("[TELEGRAM] Sistema de alertas inicializado com sucesso!");
  Serial.printf("[TELEGRAM] Bot: %s\n", botToken.substring(0, 10).c_str());
  Serial.printf("[TELEGRAM] Chat: %s\n", chatId.c_str());
  Serial.printf("[TELEGRAM] MAX_FAILURES_BEFORE_ALERT: %d\n", MAX_FAILURES_BEFORE_ALERT);
  Serial.printf("[TELEGRAM] ALERT_COOLDOWN_MS: %d\n", ALERT_COOLDOWN_MS);
  
  return true;
}

void TelegramAlerts::end() {
  isEnabled = false;
  botToken = "";
  chatId = "";
  Serial.println("[TELEGRAM] Sistema de alertas finalizado");
}

void TelegramAlerts::setBotToken(const char* token) {
  botToken = String(token);
  Serial.printf("[TELEGRAM] Token atualizado: %s\n", botToken.substring(0, 10).c_str());
}

void TelegramAlerts::setChatId(const char* id) {
  chatId = String(id);
  Serial.printf("[TELEGRAM] Chat ID atualizado: %s\n", chatId.c_str());
}

void TelegramAlerts::enable(bool enabled) {
  isEnabled = enabled;
  Serial.printf("[TELEGRAM] Alertas %s\n", enabled ? "habilitados" : "desabilitados");
}

void TelegramAlerts::updateTargetStatus(int targetIndex, Status newStatus, uint16_t latency) {
  if (!isActive() || targetIndex < 0 || targetIndex >= 6) {
    Serial.printf("[TELEGRAM] updateTargetStatus: não ativo ou índice inválido (targetIndex=%d, isActive=%s)\n", 
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
        Serial.printf("[TELEGRAM] Target %d: Início do downtime\n", targetIndex);
      }
      
      Serial.printf("[TELEGRAM] Target %d: Falha #%d\n", targetIndex, state.failure_count);
      
      // Enviar alerta se atingiu o limite
      if (state.failure_count >= MAX_FAILURES_BEFORE_ALERT && isTimeForAlert(targetIndex)) {
        Serial.printf("[TELEGRAM] Enviando alerta para target %d (falhas: %d)\n", targetIndex, state.failure_count);
        sendAlert(targetIndex, "Target", newStatus, latency);
        state.last_alert = now;
        state.alert_sent = true;
      } else {
        Serial.printf("[TELEGRAM] Ainda não é hora de alertar (falhas: %d, limite: %d)\n", 
                     state.failure_count, MAX_FAILURES_BEFORE_ALERT);
      }
    } else if (newStatus == UP && state.last_status == DOWN) {
      // Recuperação: status mudou de DOWN para UP
      Serial.printf("[TELEGRAM] Recuperação detectada para target %d\n", targetIndex);
      if (state.alert_sent && isTimeForAlert(targetIndex, true)) {
        sendRecoveryAlert(targetIndex, "Target", latency);
        state.last_alert = now;
      }
      state.failure_count = 0;
      state.alert_sent = false;
      state.downtime_start = 0;  // Reset downtime tracking
    }
    
    state.last_status = newStatus;
  } else if (newStatus == DOWN) {
    // Status continua DOWN, incrementar contador
    state.failure_count++;
    Serial.printf("[TELEGRAM] Target %d: Falha contínua #%d\n", targetIndex, state.failure_count);
    
      // Verificar se deve enviar alerta
  Serial.printf("[TELEGRAM] Verificando alerta: falhas=%d, limite=%d, isTimeForAlert=%s\n", 
               state.failure_count, MAX_FAILURES_BEFORE_ALERT, isTimeForAlert(targetIndex) ? "true" : "false");
  
  if (state.failure_count >= MAX_FAILURES_BEFORE_ALERT && isTimeForAlert(targetIndex)) {
    Serial.printf("[TELEGRAM] Enviando alerta para target %d (falhas contínuas: %d)\n", targetIndex, state.failure_count);
    sendAlert(targetIndex, "Target", newStatus, latency);
    state.last_alert = now;
    state.alert_sent = true;
  } else {
    Serial.printf("[TELEGRAM] Alerta NÃO enviado: falhas=%d, limite=%d, isTimeForAlert=%s\n", 
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
  
  String message = formatAlertMessage(realTargetName, status, latency, false, 0);
  Serial.printf("[TELEGRAM] Enviando alerta para target %d: %s\n", targetIndex, realTargetName);
  
  if (sendMessage(message)) {
    Serial.println("[TELEGRAM] Alerta enviado com sucesso!");
  } else {
    Serial.println("[TELEGRAM] Falha ao enviar alerta!");
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
  
  // Calcular tempo total de downtime
  unsigned long totalDowntime = 0;
  if (targetIndex >= 0 && targetIndex < 6 && alertStates[targetIndex].downtime_start > 0) {
    totalDowntime = (millis() - alertStates[targetIndex].downtime_start) / 1000;
  }
  
  String message = formatAlertMessage(realTargetName, UP, latency, true, totalDowntime);
  Serial.printf("[TELEGRAM] Enviando alerta de recuperação para target %d: %s (downtime: %lus)\n", 
               targetIndex, realTargetName, totalDowntime);
  
  if (sendMessage(message)) {
    Serial.println("[TELEGRAM] Alerta de recuperação enviado com sucesso!");
  } else {
    Serial.println("[TELEGRAM] Falha ao enviar alerta de recuperação!");
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
    message += "🕐 " + formatTime(millis() / 1000) + " de uptime\n";
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
    message += "🕐 " + formatTime(millis() / 1000) + " de downtime\n";
  }
  
  message += "\n🌌 _Nebula Monitor v2.2_";
  
  return message;
}

bool TelegramAlerts::sendMessage(const String& message) {
  if (!isActive() || WiFi.status() != WL_CONNECTED) {
    Serial.println("[TELEGRAM] Não é possível enviar mensagem - WiFi desconectado");
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
  
  Serial.printf("[TELEGRAM] Enviando para: %s\n", url.c_str());
  Serial.printf("[TELEGRAM] Mensagem: %s\n", message.c_str());
  
  int httpResponseCode = http.POST(jsonString);
  
  bool success = false;
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.printf("[TELEGRAM] Resposta HTTP: %d\n", httpResponseCode);
    Serial.printf("[TELEGRAM] Resposta: %s\n", response.c_str());
    
    http.end();
    success = httpResponseCode == 200;
  } else {
    Serial.printf("[TELEGRAM] Erro HTTP: %d\n", httpResponseCode);
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
    Serial.println("[HEALTH] Status healthy encontrado no payload");
    return true;
  }
  
  // Verificar se contém "status":"unhealthy" ou "status":"down"
  if (response.indexOf("\"status\":\"unhealthy\"") > 0 || 
      response.indexOf("\"status\":\"down\"") > 0) {
    Serial.println("[HEALTH] Status unhealthy encontrado no payload");
    return false;
  }
  
  // Se não encontrou status específico, considerar como não healthy
  Serial.println("[HEALTH] Status não encontrado no payload, considerando como não healthy");
  return false;
}

bool TelegramAlerts::isTimeForAlert(int targetIndex, bool isRecovery) {
  if (targetIndex < 0 || targetIndex >= 6) return false;
  
  AlertState& state = alertStates[targetIndex];
  unsigned long now = millis();
  unsigned long cooldown = isRecovery ? ALERT_RECOVERY_COOLDOWN_MS : ALERT_COOLDOWN_MS;
  
  // Se nunca enviou alerta (last_alert = 0), pode enviar
  if (state.last_alert == 0) {
    Serial.printf("[TELEGRAM] isTimeForAlert: targetIndex=%d, PRIMEIRO ALERTA (last_alert=0)\n", targetIndex);
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

void TelegramAlerts::resetFailureCount(int targetIndex) {
  if (targetIndex < 0 || targetIndex >= 6) return;
  alertStates[targetIndex].failure_count = 0;
  alertStates[targetIndex].alert_sent = false;
  Serial.printf("[TELEGRAM] Contador de falhas resetado para target %d\n", targetIndex);
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
    String message = "🚀 *Nebula Monitor v2.2*\n\n";
    message += "📊 *Monitorando:* " + String(6) + " targets\n";
    message += "🔔 *Threshold:* " + String(MAX_FAILURES_BEFORE_ALERT) + " falhas\n";
    message += "⏰ *Cooldown:* " + String(ALERT_COOLDOWN_MS / 1000) + "s\n\n";
    message += "_Tech Tweakers - 2025_";
    
    TelegramAlerts::sendMessage(message);
  }
}




