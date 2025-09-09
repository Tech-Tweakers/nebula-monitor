#include <SPIFFS.h>

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("=== VERIFICANDO SPIFFS ===");
  
  // Inicializar SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("ERRO: Falha ao inicializar SPIFFS!");
    return;
  }
  Serial.println("SPIFFS inicializado com sucesso!");
  
  // Listar arquivos
  Serial.println("\nArquivos no SPIFFS:");
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file) {
    Serial.printf("Arquivo: %s (tamanho: %d bytes)\n", file.name(), file.size());
    file = root.openNextFile();
  }
  
  // Verificar config.env
  if (SPIFFS.exists("/config.env")) {
    Serial.println("\nconfig.env encontrado! Conteúdo:");
    File configFile = SPIFFS.open("/config.env", "r");
    if (configFile) {
      while (configFile.available()) {
        String line = configFile.readStringUntil('\n');
        if (line.indexOf("TARGET") != -1) {
          Serial.println(line);
        }
      }
      configFile.close();
    }
  } else {
    Serial.println("\nconfig.env NÃO encontrado!");
  }
}

void loop() {
  delay(1000);
}
