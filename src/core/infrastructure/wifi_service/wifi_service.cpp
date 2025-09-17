#include "wifi_service.h"
#include "core/infrastructure/logger/logger.h"

WiFiService::WiFiService() : connected(false), lastReconnectAttempt(0) {
}

bool WiFiService::initialize(const String& ssid, const String& password) {
  this->ssid = ssid;
  this->password = password;
  
  Serial_println("[WIFI] Initializing WiFi service...");
  
  WiFi.mode(WIFI_STA);
  
  // Configure DNS servers before connecting
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, IPAddress(8, 8, 8, 8), IPAddress(8, 8, 4, 4));
  Serial_println("[WIFI] DNS configured: 8.8.8.8, 8.8.4.4");
  
  WiFi.begin(ssid.c_str(), password.c_str());
  
  // Wait a bit for initial connection
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < 10000) {
    delay(500);
    Serial_print(".");
  }
  
  updateConnectionStatus();
  
  if (connected) {
    Serial_println("\n[WIFI] Connected successfully!");
    
    // Try to set DNS after connection (some routers override DNS)
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, IPAddress(8, 8, 8, 8), IPAddress(8, 8, 4, 4));
    Serial_println("[WIFI] DNS reconfigured after connection");
    
    printInfo();
  } else {
    Serial_println("\n[WIFI] Initial connection failed, will retry...");
  }
  
  return true; // Always return true, connection will be retried
}

void WiFiService::update() {
  updateConnectionStatus();
  
  // Auto-reconnect if disconnected
  if (!connected && (millis() - lastReconnectAttempt) > RECONNECT_INTERVAL) {
    Serial_println("[WIFI] Attempting to reconnect...");
    reconnect();
    lastReconnectAttempt = millis();
  }
}

bool WiFiService::connect() {
  if (ssid.length() == 0 || password.length() == 0) {
    Serial_println("[WIFI] ERROR: SSID or password not set!");
    return false;
  }
  
  // Configure DNS servers before connecting
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, IPAddress(8, 8, 8, 8), IPAddress(8, 8, 4, 4));
  
  WiFi.begin(ssid.c_str(), password.c_str());
  
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < 15000) {
    delay(250);
  }
  
  updateConnectionStatus();
  return connected;
}

bool WiFiService::reconnect() {
  if (connected) return true;
  
  Serial_println("[WIFI] Reconnecting...");
  WiFi.disconnect();
  delay(100);
  return connect();
}

void WiFiService::disconnect() {
  WiFi.disconnect();
  connected = false;
  Serial_println("[WIFI] Disconnected");
}

void WiFiService::updateConnectionStatus() {
  bool wasConnected = connected;
  connected = (WiFi.status() == WL_CONNECTED);
  
  if (connected && !wasConnected) {
    Serial_println("[WIFI] Connected!");
    printInfo();
  } else if (!connected && wasConnected) {
    Serial_println("[WIFI] Disconnected!");
  }
}

String WiFiService::getLocalIP() const {
  return connected ? WiFi.localIP().toString() : "0.0.0.0";
}

String WiFiService::getGatewayIP() const {
  return connected ? WiFi.gatewayIP().toString() : "0.0.0.0";
}

String WiFiService::getSubnetMask() const {
  return connected ? WiFi.subnetMask().toString() : "0.0.0.0";
}

String WiFiService::getDNSIP() const {
  return connected ? WiFi.dnsIP(0).toString() : "0.0.0.0";
}

int WiFiService::getRSSI() const {
  return connected ? WiFi.RSSI() : -999;
}

void WiFiService::printInfo() const {
  if (!connected) {
    Serial_println("[WIFI] Not connected");
    return;
  }
  
  Serial_printf("[WIFI] IP=%s  GW=%s  MASK=%s  DNS0=%s  RSSI=%d dBm\n",
    getLocalIP().c_str(),
    getGatewayIP().c_str(),
    getSubnetMask().c_str(),
    getDNSIP().c_str(),
    getRSSI());
  
  // Show configured DNS vs actual DNS
  Serial_printf("[WIFI] Configured DNS: 8.8.8.8, 8.8.4.4 | Actual DNS: %s\n", getDNSIP().c_str());
}
