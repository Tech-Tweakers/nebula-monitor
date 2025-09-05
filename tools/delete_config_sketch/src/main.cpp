#include <Arduino.h>
#include <SPIFFS.h>

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("========================================");
  Serial.println("    DELETAR CONFIG.ENV DO SPIFFS");
  Serial.println("========================================");
  
  // Inicializar SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("ERRO: Falha ao inicializar SPIFFS!");
    return;
  }
  
  // Verificar se arquivo existe
  if (SPIFFS.exists("/config.env")) {
    Serial.println("Arquivo config.env encontrado no SPIFFS");
    
    // Deletar arquivo
    if (SPIFFS.remove("/config.env")) {
      Serial.println("SUCESSO: config.env deletado do SPIFFS!");
    } else {
      Serial.println("ERRO: Falha ao deletar config.env");
    }
  } else {
    Serial.println("Arquivo config.env NÃO encontrado no SPIFFS");
  }
  
  // Listar arquivos restantes
  Serial.println("\nArquivos restantes no SPIFFS:");
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file) {
    Serial.printf("  %s (%d bytes)\n", file.name(), file.size());
    file = root.openNextFile();
  }
  
  Serial.println("========================================");
}

void loop() {
  // Nada a fazer
  delay(1000);
}
