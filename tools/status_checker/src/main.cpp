#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <HTTPClient.h>

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("========================================");
  Serial.println("    STATUS CHECKER - NEBULA MONITOR");
  Serial.println("========================================");
  
  // Inicializar SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("ERRO: Falha ao inicializar SPIFFS!");
    return;
  }
  
  // Inicializar ConfigManager
  if (!ConfigManager::begin()) {
    Serial.println("ERRO: Falha ao inicializar ConfigManager!");
    return;
  }
  
  Serial.println("ConfigManager inicializado!");
  ConfigManager::printAllConfigs();
  
  // Conectar WiFi
  Serial.println("\nConectando ao WiFi...");
  WiFi.begin(ConfigManager::getWifiSSID().c_str(), ConfigManager::getWifiPass().c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi conectado!");
    Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
  } else {
    Serial.println("\nERRO: Falha ao conectar WiFi!");
    return;
  }
  
  // Mostrar targets configurados
  Serial.println("\n========================================");
  Serial.println("    TARGETS CONFIGURADOS");
  Serial.println("========================================");
  
  int targetCount = ConfigManager::getTargetCount();
  Serial.printf("Total de targets: %d\n\n", targetCount);
  
  for (int i = 0; i < targetCount; i++) {
    String name = ConfigManager::getTargetName(i);
    String url = ConfigManager::getTargetUrl(i);
    String healthEndpoint = ConfigManager::getTargetHealthEndpoint(i);
    String monitorType = ConfigManager::getTargetMonitorType(i);
    
    Serial.printf("Target %d:\n", i + 1);
    Serial.printf("  Nome: %s\n", name.c_str());
    Serial.printf("  URL: %s\n", url.c_str());
    Serial.printf("  Health Endpoint: %s\n", healthEndpoint.length() > 0 ? healthEndpoint.c_str() : "N/A");
    Serial.printf("  Tipo: %s\n", monitorType.c_str());
    Serial.println();
  }
  
  Serial.println("========================================");
  Serial.println("    TESTE DE CONECTIVIDADE");
  Serial.println("========================================");
  
  // Testar cada target
  for (int i = 0; i < targetCount; i++) {
    String name = ConfigManager::getTargetName(i);
    String url = ConfigManager::getTargetUrl(i);
    String healthEndpoint = ConfigManager::getTargetHealthEndpoint(i);
    String monitorType = ConfigManager::getTargetMonitorType(i);
    
    Serial.printf("Testando %s (%s)...\n", name.c_str(), url.c_str());
    
    // Fazer requisição HTTP simples
    HTTPClient http;
    http.begin(url.c_str());
    http.setTimeout(5000);
    http.setUserAgent("NebulaWatch/1.0");
    
    int httpCode = http.GET();
    
    if (httpCode > 0) {
      Serial.printf("  ✅ Status: %d (OK)\n", httpCode);
    } else {
      Serial.printf("  ❌ Erro: %d\n", httpCode);
    }
    
    http.end();
    delay(1000); // Pausa entre testes
  }
  
  Serial.println("\n========================================");
  Serial.println("    FIM DO TESTE");
  Serial.println("========================================");
}

void loop() {
  delay(1000);
}
