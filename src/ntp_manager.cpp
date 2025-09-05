#include "ntp_manager.hpp"
#include "config_manager.hpp"

// Inicialização dos membros estáticos
WiFiUDP NTPManager::ntpUDP;
NTPClient NTPManager::timeClient(ntpUDP);
bool NTPManager::initialized = false;
bool NTPManager::timeSynced = false;
unsigned long NTPManager::lastSyncTime = 0;

bool NTPManager::begin() {
    if (initialized) return true;
    
    Serial.println("[NTP] Init NTP Manager...");
    
    // Configurar servidor NTP (pool.ntp.org é mais confiável)
    timeClient.setTimeOffset(getTimezoneOffset());
    timeClient.setUpdateInterval(SYNC_INTERVAL);
    
    initialized = true;
    Serial.println("[NTP] NTP Manager started!");
    
    return true;
}

void NTPManager::end() {
    if (initialized) {
        timeClient.end();
        initialized = false;
        timeSynced = false;
        Serial.println("[NTP] NTP Manager finished");
    }
}

bool NTPManager::syncTime() {
    if (!initialized) {
        Serial.println("[NTP] ERROR: NTP Manager not started!");
        return false;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[NTP] ERROR: WiFi failed!");
        return false;
    }
    
    Serial.println("[NTP] Sync with NTP server...");
    
    // Tentar sincronizar
    if (timeClient.forceUpdate()) {
        timeSynced = true;
        lastSyncTime = millis();
        
        // Configurar timezone do sistema
        setTimezone(getTimezoneOffset());
        
        Serial.printf("[NTP] Synced! Actual time: %s\n", 
                     formatTime(getCurrentTime()).c_str());
        return true;
    } else {
        Serial.println("[NTP] ERROR: Failed to sync NTP!");
        return false;
    }
}

bool NTPManager::isTimeSynced() {
    return timeSynced && (millis() - lastSyncTime < SYNC_INTERVAL);
}

time_t NTPManager::getCurrentTime() {
    if (!isTimeSynced()) {
        // Tentar sincronizar se não estiver sincronizado
        syncTime();
    }
    
    return timeClient.getEpochTime();
}

time_t NTPManager::getTimeFromString(const String& dateTime) {
    // Formato esperado: "2024-01-15 14:30:25"
    // Implementação simples para parsing de data
    if (dateTime.length() < 19) return 0;
    
    // Extrair componentes da data
    int year = dateTime.substring(0, 4).toInt();
    int month = dateTime.substring(5, 7).toInt();
    int day = dateTime.substring(8, 10).toInt();
    int hour = dateTime.substring(11, 13).toInt();
    int minute = dateTime.substring(14, 16).toInt();
    int second = dateTime.substring(17, 19).toInt();
    
    // Criar struct tm
    struct tm timeinfo = {0};
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = month - 1;
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = second;
    
    // Converter para timestamp Unix
    return mktime(&timeinfo);
}

String NTPManager::formatTime(time_t timestamp) {
    if (timestamp == 0) return "N/A";
    
    struct tm* timeinfo = localtime(&timestamp);
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return String(buffer);
}

bool NTPManager::forceSync() {
    timeSynced = false;
    return syncTime();
}

bool NTPManager::needsSync() {
    return !isTimeSynced() || (millis() - lastSyncTime > SYNC_INTERVAL);
}

int NTPManager::getTimezoneOffset() {
    // Obter offset do config.env, padrão -3 (Brasil)
    // Usar valor padrão se ConfigManager não estiver disponível
    return -10800; // UTC-3 (Brasil)
}

void NTPManager::setTimezone(int offsetSeconds) {
    // Configurar timezone do sistema ESP32
    configTime(offsetSeconds, 0, "pool.ntp.org");
    Serial.printf("[NTP] Timezone configured: %d seconds\n", offsetSeconds);
}
