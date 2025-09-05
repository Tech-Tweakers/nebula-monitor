#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

class NTPManager {
private:
    static WiFiUDP ntpUDP;
    static NTPClient timeClient;
    static bool initialized;
    static bool timeSynced;
    static unsigned long lastSyncTime;
    static const unsigned long SYNC_INTERVAL = 3600000; // 1 hora
    
    static void setTimezone(int offsetSeconds);
    
public:
    static bool begin();
    static void end();
    
    // Sincronizar com servidor NTP
    static bool syncTime();
    
    // Verificar se tempo está sincronizado
    static bool isTimeSynced();
    
    // Obter timestamp Unix atual
    static time_t getCurrentTime();
    
    // Obter timestamp Unix de uma data específica
    static time_t getTimeFromString(const String& dateTime);
    
    // Formatar timestamp para string legível
    static String formatTime(time_t timestamp);
    
    // Forçar nova sincronização
    static bool forceSync();
    
    // Verificar se precisa sincronizar
    static bool needsSync();
    
    // Obter offset de timezone do config
    static int getTimezoneOffset();
};
