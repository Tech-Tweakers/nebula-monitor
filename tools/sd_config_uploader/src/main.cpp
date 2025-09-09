#include <Arduino.h>
#include <SD.h>
#include <SPIFFS.h>

// Pin para CS do SD Card (mesmo do projeto principal)
const int SD_CS_PIN = SD_CS_PIN_VALUE;

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("========================================");
  Serial.println("    CONFIG UPLOADER PARA SD CARD");
  Serial.println("========================================");
  
  // Inicializar SPIFFS primeiro
  Serial.println("[UPLOADER] Inicializando SPIFFS...");
  if (!SPIFFS.begin(true)) {
    Serial.println("[UPLOADER] ERRO: Falha ao inicializar SPIFFS!");
    return;
  }
  Serial.println("[UPLOADER] SPIFFS inicializado com sucesso!");
  
  // Inicializar SD Card
  Serial.println("[UPLOADER] Inicializando SD Card...");
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("[UPLOADER] ERRO: Falha ao inicializar SD Card!");
    Serial.println("[UPLOADER] Verifique se o SD está inserido e funcionando.");
    return;
  }
  Serial.println("[UPLOADER] SD Card inicializado com sucesso!");
  
  // Verificar se config.env existe no SPIFFS (na mesma pasta do sketch)
  if (!SPIFFS.exists("/config.env")) {
    Serial.println("[UPLOADER] ERRO: config.env não encontrado no SPIFFS!");
    Serial.println("[UPLOADER] Certifique-se de que o arquivo está na pasta do uploader");
    return;
  }
  
  // Abrir arquivo do SPIFFS (que contém o config.env da pasta do uploader)
  File sourceFile = SPIFFS.open("/config.env", "r");
  if (!sourceFile) {
    Serial.println("[UPLOADER] ERRO: Não foi possível abrir config.env!");
    return;
  }
  
  // Criar arquivo no SD
  File destFile = SD.open("/config.env", FILE_WRITE);
  if (!destFile) {
    Serial.println("[UPLOADER] ERRO: Não foi possível criar config.env no SD!");
    sourceFile.close();
    return;
  }
  
  // Copiar arquivo byte por byte (sem modificações)
  Serial.println("[UPLOADER] Copiando config.env da pasta do uploader para o SD Card...");
  
  uint8_t buffer[512];
  size_t totalBytes = 0;
  
  while (sourceFile.available()) {
    size_t bytesRead = sourceFile.read(buffer, sizeof(buffer));
    if (bytesRead == 0) break;
    
    size_t bytesWritten = destFile.write(buffer, bytesRead);
    if (bytesWritten != bytesRead) {
      Serial.println("[UPLOADER] ERRO: Falha ao escrever no SD Card!");
      sourceFile.close();
      destFile.close();
      return;
    }
    
    totalBytes += bytesWritten;
  }
  
  sourceFile.close();
  destFile.close();
  
  Serial.println();
  Serial.printf("[UPLOADER] SUCESSO! %d bytes copiados para o SD Card!\n", totalBytes);
  
  // Verificar se arquivo foi criado corretamente
  if (SD.exists("/config.env")) {
    File verifyFile = SD.open("/config.env", FILE_READ);
    if (verifyFile) {
      Serial.printf("[UPLOADER] Verificação: Arquivo no SD tem %d bytes\n", verifyFile.size());
      verifyFile.close();
    }
  }
  
  Serial.println("[UPLOADER] Configuração copiada com sucesso!");
  Serial.println("[UPLOADER] Agora você pode:");
  Serial.println("  1. Inserir o SD no ESP32 principal");
  Serial.println("  2. Executar o projeto principal");
  Serial.println("  3. O ESP32 vai detectar e sincronizar automaticamente!");
  Serial.println("========================================");
}

void loop() {
  // Nada a fazer aqui
  delay(1000);
}
