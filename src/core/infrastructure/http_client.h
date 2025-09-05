#pragma once
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Arduino.h>

class HttpClient {
private:
  HTTPClient http;
  String lastResponse;
  
public:
  HttpClient();
  ~HttpClient();
  
  // Basic HTTP operations
  uint16_t ping(const String& url, uint16_t timeout = 5000);
  uint16_t healthCheck(const String& url, const String& endpoint, uint16_t timeout = 7000);
  String get(const String& url, uint16_t timeout = 5000);
  String post(const String& url, const String& data, uint16_t timeout = 5000);
  
  // Response handling
  String getLastResponse() const { return lastResponse; }
  bool isHealthyResponse(const String& response) const;
  
  // Utility methods
  void setUserAgent(const String& userAgent);
  void addHeader(const String& name, const String& value);
  void clearHeaders();
  
private:
  uint16_t performRequest(const String& url, uint16_t timeout, const String& method = "GET", const String& data = "");
  bool isHttpsUrl(const String& url) const;
  void setupSecureClient(WiFiClientSecure& client, const String& url) const;
  void setupHeaders(const String& url);
};
