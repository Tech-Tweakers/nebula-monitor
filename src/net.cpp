#include "net.hpp"
#include <time.h>

namespace Net {

void connectWiFi(const char* ssid, const char* pass){
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  uint32_t t0 = millis();
  while (WiFi.status()!=WL_CONNECTED && millis()-t0<15000){ delay(250); Serial.print("."); }
  Serial.println(WiFi.status()==WL_CONNECTED ? "\nWiFi OK" : "\nWiFi falhou");
}

void printInfo(){
  Serial.printf("[NET] IP=%s  GW=%s  MASK=%s  DNS=%s  RSSI=%d dBm\n",
    WiFi.localIP().toString().c_str(),
    WiFi.gatewayIP().toString().c_str(),
    WiFi.subnetMask().toString().c_str(),
    WiFi.dnsIP().toString().c_str(),
    WiFi.RSSI());
}

void forceDNS(IPAddress dns1, IPAddress dns2){
  if (WiFi.status()!=WL_CONNECTED) return;
  IPAddress ip=WiFi.localIP(), gw=WiFi.gatewayIP(), mask=WiFi.subnetMask();
  bool ok = WiFi.config(ip, gw, mask, dns1, dns2);
  Serial.printf("[DNS] set %s, %s  (config=%s)\n",
    dns1.toString().c_str(), dns2.toString().c_str(), ok?"OK":"FAIL");
  WiFi.disconnect(false, false); delay(150); WiFi.reconnect();
  uint32_t t0 = millis();
  while (WiFi.status()!=WL_CONNECTED && millis()-t0<8000) delay(200);
  Serial.printf("[DNS] now: %s | %s\n",
    WiFi.dnsIP(0).toString().c_str(), WiFi.dnsIP(1).toString().c_str());
}

bool ntpSync(uint32_t timeout_ms){
  configTime(-3*3600, 0, "time.cloudflare.com", "pool.ntp.org");
  uint32_t t0 = millis(); time_t now; struct tm tm{};
  do { delay(200); time(&now); localtime_r(&now,&tm); }
  while ((tm.tm_year + 1900) < 2022 && (millis()-t0) < timeout_ms);
  return (tm.tm_year + 1900) >= 2022;
}

uint16_t httpPing(const char* url, uint16_t base_timeout_ms){
  if (WiFi.status()!=WL_CONNECTED) return 0;

  uint16_t to_ms = base_timeout_ms;
  if (strstr(url, "ngrok-free.app") || strstr(url, "trycloudflare.com")) to_ms = 7000;

  uint32_t t0 = millis();
  int code = -1;
  auto follow = HTTPC_STRICT_FOLLOW_REDIRECTS;

  if (strncmp(url,"https://",8)==0){
    WiFiClientSecure client; client.setInsecure();
    client.setTimeout((to_ms+999)/1000);
    HTTPClient https; https.setTimeout(to_ms); https.setFollowRedirects(follow);
    if (https.begin(client, url)){
      https.addHeader("User-Agent","PolarisWatchdog/1.0");
      https.addHeader("Accept","*/*");
      code = https.GET();
      if (code <= 0) code = https.sendRequest("HEAD");
      https.end();
    }
  } else {
    WiFiClient client; client.setTimeout((to_ms+999)/1000);
    HTTPClient http; http.setTimeout(to_ms); http.setFollowRedirects(follow);
    if (http.begin(client, url)){
      http.addHeader("User-Agent","PolarisWatchdog/1.0");
      http.addHeader("Accept","*/*");
      code = http.GET();
      if (code <= 0) code = http.sendRequest("HEAD");
      http.end();
    }
  }

  uint32_t dt = millis() - t0;
  Serial.printf("[HTTP] %s -> code=%d (%lums)\n", url, code, (unsigned long)dt);

  if (code > 0 && code != 400) { if (dt>65535) dt=65535; return (uint16_t)dt; }
  return 0;
}

} // namespace Net
