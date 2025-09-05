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
  
  // Mostrar espaço total e usado
  Serial.printf("\nEspaço total: %d bytes\n", SPIFFS.totalBytes());
  Serial.printf("Espaço usado: %d bytes\n", SPIFFS.usedBytes());
  Serial.printf("Espaço livre: %d bytes\n", SPIFFS.totalBytes() - SPIFFS.usedBytes());
  
  Serial.println("========================================");
}

void loop() {
  delay(1000);
}
