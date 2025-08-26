#pragma once
#include <Arduino.h>
#include <IPAddress.h>

namespace Net {
  bool     connectWiFi(const char* ssid, const char* pass, uint32_t timeout_ms=15000);
  void     printInfo();
  void     forceDNS(IPAddress dns1 = IPAddress(8,8,8,8), IPAddress dns2 = IPAddress(1,1,1,1));
  bool     ntpSync(uint32_t timeout_ms = 7000);
  uint16_t httpPing(const char* url, uint16_t timeout_ms = 2500); // “ping-like”: qualquer código !=400
}
