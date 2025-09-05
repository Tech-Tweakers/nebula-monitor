#pragma once
#include <Arduino.h>
#include <SD.h>
#include <SPIFFS.h>
#include "ntp_manager.hpp"

class SDConfigManager {
private:
    static bool sdInitialized;
    static const char* CONFIG_FILE;
    static const int SD_CS_PIN = 5; // Pin para CS do SD Card
    
    static bool initSD();
    static time_t getFileTimestamp(File& file);
    static bool copyFile(File& source, File& destination);
    static bool filesAreIdentical(File& file1, File& file2);
    static String getFileHash(File& file);
    
public:
    static bool begin();
    static void end();
    
    // Verifica se SD tem config mais nova que SPIFFS
    static bool hasNewerConfig();
    
    // Copia config do SD para SPIFFS
    static bool copySDToSPIFFS();
    
    // Verifica se SD está disponível
    static bool isSDAvailable();
    
    // Obtém timestamp do arquivo config no SD
    static time_t getSDConfigTimestamp();
    
    // Obtém timestamp do arquivo config no SPIFFS
    static time_t getSPIFFSConfigTimestamp();
};
