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
  
  // Verificar se config.env existe no SPIFFS
  if (!SPIFFS.exists("/config.env")) {
    Serial.println("[UPLOADER] ERRO: config.env não encontrado no SPIFFS!");
    Serial.println("[UPLOADER] Execute primeiro: pio run --target uploadfs");
    return;
  }
  
  // Abrir arquivo do SPIFFS
  File sourceFile = SPIFFS.open("/config.env", "r");
  if (!sourceFile) {
    Serial.println("[UPLOADER] ERRO: Não foi possível abrir config.env do SPIFFS!");
    return;
  }
  
  // Criar arquivo no SD
  File destFile = SD.open("/config.env", FILE_WRITE);
  if (!destFile) {
    Serial.println("[UPLOADER] ERRO: Não foi possível criar config.env no SD!");
    sourceFile.close();
    return;
  }
  
  // Ler arquivo completo do SPIFFS
  String configContent = sourceFile.readString();
  sourceFile.close();
  
  // Modificar o conteúdo (mudar "Proxmox HV" para "Proxmox VM")
  configContent.replace("Proxmox", "Proxmox VM");
  
  Serial.println("[UPLOADER] Modificando config.env (Proxmox HV -> Proxmox VM)...");
  
  // Escrever conteúdo modificado no SD
  size_t bytesWritten = destFile.print(configContent);
  size_t bytesRead = bytesWritten;
  
  destFile.close();
  
  Serial.println();
  Serial.printf("[UPLOADER] SUCESSO! %d bytes copiados para o SD Card!\n", bytesRead);
  
  // Verificar se arquivo foi criado corretamente
  if (SD.exists("/config.env")) {
    File verifyFile = SD.open("/config.env", FILE_READ);
    if (verifyFile) {
      Serial.printf("[UPLOADER] Verificação: Arquivo no SD tem %d bytes\n", verifyFile.size());
      verifyFile.close();
    }
  }
  
  Serial.println("[UPLOADER] Configuração MODIFICADA e copiada com sucesso!");
  Serial.println("[UPLOADER] Mudança: 'Proxmox HV' -> 'Proxmox VM'");
  Serial.println("[UPLOADER] Agora você pode:");
  Serial.println("  1. Inserir o SD no ESP32 principal");
  Serial.println("  2. Executar o projeto principal");
  Serial.println("  3. O ESP32 vai detectar a mudança e reiniciar!");
  Serial.println("  4. Ver o target 'Proxmox VM' na tela!");
  Serial.println("========================================");
}

void loop() {
  // Nada a fazer aqui
  delay(1000);
}
