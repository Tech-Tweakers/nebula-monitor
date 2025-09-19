#pragma once
#include <WiFi.h>
#include <Arduino.h>

class WiFiService {
private:
  String ssid;
  String password;
  bool connected;
  unsigned long lastReconnectAttempt;
  static const unsigned long RECONNECT_INTERVAL = 30000; // 30 seconds
  
public:
  WiFiService();
  
  // Initialization
  bool initialize(const String& ssid, const String& password);
  void update();
  
  // Connection management
  bool connect();
  bool reconnect();
  void disconnect();
  
  // Status
  bool isConnected() const { return connected; }
  bool isDisconnected() const { return !connected; }
  
  // Getters
  String getLocalIP() const;
  String getGatewayIP() const;
  String getSubnetMask() const;
  String getDNSIP() const;
  int getRSSI() const;
  String getSSID() const { return ssid; }
  
  // Network info
  void printInfo() const;
  
private:
  void updateConnectionStatus();
};
