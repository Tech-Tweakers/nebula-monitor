#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

namespace Net {
  void connectWiFi(const char* ssid, const char* pass);
  void forceDNS(IPAddress dns1 = IPAddress(8,8,8,8), IPAddress dns2 = IPAddress(1,1,1,1));
  bool ntpSync(uint32_t timeout_ms = 7000);
  void printInfo();
  uint16_t httpPing(const char* url, uint16_t base_timeout_ms = 2500);
}
