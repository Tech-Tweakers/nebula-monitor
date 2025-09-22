#include <Arduino.h>
#include <SPIFFS.h>

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("========================================");
  Serial.println("    CONTEÚDO DO SPIFFS");
  Serial.println("========================================");
  
  // Inicializar SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("ERRO: Falha ao inicializar SPIFFS!");
    return;
  }
  
  // Listar arquivos
  Serial.println("Arquivos no SPIFFS:");
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  int count = 0;
  
  while (file) {
    count++;
    Serial.printf("  %d. %s (%d bytes)\n", count, file.name(), file.size());
    file = root.openNextFile();
  }
  
  if (count == 0) {
    Serial.println("  SPIFFS vazio!");
  } else {
    Serial.printf("\nTotal: %d arquivos\n", count);
  }
  
  // Mostrar conteúdo do config.env
  Serial.println("\n========================================");
  Serial.println("    CONTEÚDO DO config.env");
  Serial.println("========================================");
  
  if (SPIFFS.exists("/config.env")) {
    File configFile = SPIFFS.open("/config.env", "r");
    if (configFile) {
      Serial.printf("Arquivo: %s (%d bytes)\n", configFile.name(), configFile.size());
      Serial.println("----------------------------------------");
      
      int lineNumber = 1;
      while (configFile.available()) {
        String line = configFile.readStringUntil('\n');
        line.trim();
        Serial.printf("%3d| %s\n", lineNumber, line.c_str());
        lineNumber++;
        
        // Delay pequeno para evitar overflow do buffer serial
        if (lineNumber % 10 == 0) {
          delay(100);
        }
      }
      
      configFile.close();
      Serial.println("----------------------------------------");
    } else {
      Serial.println("ERRO: Não foi possível abrir config.env");
    }
  } else {
    Serial.println("AVISO: config.env não encontrado no SPIFFS");
  }
  
  // Mostrar espaço total e usado
  Serial.printf("\nEspaço total: %d bytes\n", SPIFFS.totalBytes());
  Serial.printf("Espaço usado: %d bytes\n", SPIFFS.usedBytes());
  Serial.printf("Espaço livre: %d bytes\n", SPIFFS.totalBytes() - SPIFFS.usedBytes());
  
  Serial.println("========================================");
}

void loop() {
  delay(1000);
}
