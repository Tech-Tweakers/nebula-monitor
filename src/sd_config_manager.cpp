#include "sd_config_manager.hpp"

// Inicialização dos membros estáticos
bool SDConfigManager::sdInitialized = false;
const char* SDConfigManager::CONFIG_FILE = "/config.env";

bool SDConfigManager::initSD() {
    if (sdInitialized) return true;
    
    Serial.println("[SD_CONFIG] Inicializando SD Card...");
    
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("[SD_CONFIG] ERRO: Falha ao inicializar SD Card");
        return false;
    }
    
    sdInitialized = true;
    Serial.println("[SD_CONFIG] SD Card inicializado com sucesso");
    return true;
}

bool SDConfigManager::begin() {
    Serial.println("[SD_CONFIG] Iniciando SDConfigManager...");
    
    // Inicializar SPIFFS primeiro (sempre necessário)
    if (!SPIFFS.begin(true)) {
        Serial.println("[SD_CONFIG] ERRO: Falha ao inicializar SPIFFS");
        return false;
    }
    
    // Inicializar NTP Manager para timestamps reais
    if (!NTPManager::begin()) {
        Serial.println("[SD_CONFIG] AVISO: Falha ao inicializar NTP, usando timestamps locais");
    }
    
    // Tentar sincronizar tempo se WiFi estiver conectado
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("[SD_CONFIG] WiFi conectado, sincronizando com NTP...");
        NTPManager::syncTime();
    } else {
        Serial.println("[SD_CONFIG] WiFi não conectado, usando timestamps locais");
    }
    
    // Tentar inicializar SD (opcional)
    initSD();
    
    return true;
}

void SDConfigManager::end() {
    if (sdInitialized) {
        SD.end();
        sdInitialized = false;
        Serial.println("[SD_CONFIG] SD Card finalizado");
    }
}

bool SDConfigManager::isSDAvailable() {
    return sdInitialized && SD.exists(CONFIG_FILE);
}

time_t SDConfigManager::getFileTimestamp(File& file) {
    if (!file) return 0;
    
    // Tentar usar getLastWrite() primeiro
    time_t timestamp = file.getLastWrite();
    
    if (timestamp == 0) {
        // Fallback: usar tempo atual do NTP se disponível
        if (NTPManager::isTimeSynced()) {
            timestamp = NTPManager::getCurrentTime();
        } else {
            // Último recurso: tempo local
            timestamp = time(nullptr);
        }
    }
    
    return timestamp;
}

time_t SDConfigManager::getSDConfigTimestamp() {
    if (!isSDAvailable()) return 0;
    
    File file = SD.open(CONFIG_FILE, FILE_READ);
    if (!file) return 0;
    
    time_t timestamp = getFileTimestamp(file);
    file.close();
    
    Serial.printf("[SD_CONFIG] Timestamp SD: %lu (%s)\n", timestamp, NTPManager::formatTime(timestamp).c_str());
    return timestamp;
}

time_t SDConfigManager::getSPIFFSConfigTimestamp() {
    if (!SPIFFS.exists(CONFIG_FILE)) return 0;
    
    File file = SPIFFS.open(CONFIG_FILE, "r");
    if (!file) return 0;
    
    time_t timestamp = getFileTimestamp(file);
    file.close();
    
    Serial.printf("[SD_CONFIG] Timestamp SPIFFS: %lu (%s)\n", timestamp, NTPManager::formatTime(timestamp).c_str());
    return timestamp;
}

bool SDConfigManager::hasNewerConfig() {
    if (!isSDAvailable()) {
        Serial.println("[SD_CONFIG] SD não disponível, usando SPIFFS");
        return false;
    }
    
    // Abrir arquivos para comparação
    File sdFile = SD.open(CONFIG_FILE, FILE_READ);
    if (!sdFile) {
        Serial.println("[SD_CONFIG] SD config inválida, usando SPIFFS");
        return false;
    }
    
    File spiffsFile = SPIFFS.open(CONFIG_FILE, "r");
    if (!spiffsFile) {
        Serial.println("[SD_CONFIG] SPIFFS sem config, copiando do SD");
        sdFile.close();
        return true;
    }
    
    // Comparar conteúdo dos arquivos
    bool identical = filesAreIdentical(sdFile, spiffsFile);
    
    sdFile.close();
    spiffsFile.close();
    
    if (identical) {
        Serial.println("[SD_CONFIG] Arquivos idênticos, não precisa copiar");
        return false;
    }
    
    // Se diferentes, verificar timestamps
    time_t sdTime = getSDConfigTimestamp();
    time_t spiffsTime = getSPIFFSConfigTimestamp();
    
    bool isNewer = sdTime > spiffsTime;
    Serial.printf("[SD_CONFIG] Arquivos diferentes, SD mais novo: %s\n", isNewer ? "SIM" : "NÃO");
    Serial.printf("[SD_CONFIG] SD: %lu (%s)\n", sdTime, NTPManager::formatTime(sdTime).c_str());
    Serial.printf("[SD_CONFIG] SPIFFS: %lu (%s)\n", spiffsTime, NTPManager::formatTime(spiffsTime).c_str());
    
    return isNewer;
}

bool SDConfigManager::copyFile(File& source, File& destination) {
    if (!source || !destination) return false;
    
    size_t bytesRead = 0;
    uint8_t buffer[512];
    
    Serial.println("[SD_CONFIG] Copiando arquivo...");
    
    while (source.available()) {
        size_t bytesToRead = source.read(buffer, sizeof(buffer));
        if (bytesToRead == 0) break;
        
        size_t bytesWritten = destination.write(buffer, bytesToRead);
        if (bytesWritten != bytesToRead) {
            Serial.println("[SD_CONFIG] ERRO: Falha na escrita");
            return false;
        }
        
        bytesRead += bytesWritten;
    }
    
    Serial.printf("[SD_CONFIG] Arquivo copiado: %d bytes\n", bytesRead);
    return true;
}

bool SDConfigManager::copySDToSPIFFS() {
    if (!isSDAvailable()) {
        Serial.println("[SD_CONFIG] ERRO: SD não disponível para cópia");
        return false;
    }
    
    File sourceFile = SD.open(CONFIG_FILE, FILE_READ);
    if (!sourceFile) {
        Serial.println("[SD_CONFIG] ERRO: Não foi possível abrir arquivo no SD");
        return false;
    }
    
    File destFile = SPIFFS.open(CONFIG_FILE, "w");
    if (!destFile) {
        Serial.println("[SD_CONFIG] ERRO: Não foi possível criar arquivo no SPIFFS");
        sourceFile.close();
        return false;
    }
    
    bool success = copyFile(sourceFile, destFile);
    
    sourceFile.close();
    destFile.close();
    
    if (success) {
        Serial.println("[SD_CONFIG] Config copiada do SD para SPIFFS com sucesso!");
    } else {
        Serial.println("[SD_CONFIG] ERRO: Falha na cópia do SD para SPIFFS");
    }
    
    return success;
}

bool SDConfigManager::filesAreIdentical(File& file1, File& file2) {
    if (!file1 || !file2) return false;
    
    // Verificar tamanho primeiro
    if (file1.size() != file2.size()) {
        Serial.printf("[SD_CONFIG] Tamanhos diferentes: %d vs %d\n", file1.size(), file2.size());
        return false;
    }
    
    // Comparar byte por byte
    file1.seek(0);
    file2.seek(0);
    
    uint8_t buffer1[512];
    uint8_t buffer2[512];
    
    while (file1.available()) {
        size_t bytes1 = file1.read(buffer1, sizeof(buffer1));
        size_t bytes2 = file2.read(buffer2, sizeof(buffer2));
        
        if (bytes1 != bytes2) {
            Serial.println("[SD_CONFIG] Número de bytes lidos diferentes");
            return false;
        }
        
        if (memcmp(buffer1, buffer2, bytes1) != 0) {
            Serial.println("[SD_CONFIG] Conteúdo diferente encontrado");
            return false;
        }
    }
    
    Serial.println("[SD_CONFIG] Arquivos são idênticos");
    return true;
}

String SDConfigManager::getFileHash(File& file) {
    if (!file) return "";
    
    // Hash simples usando tamanho + alguns bytes
    file.seek(0);
    size_t size = file.size();
    uint8_t firstBytes[16];
    size_t bytesRead = file.read(firstBytes, sizeof(firstBytes));
    
    String hash = String(size) + "_";
    for (int i = 0; i < bytesRead; i++) {
        hash += String(firstBytes[i], HEX);
    }
    
    return hash;
}
