#include "http_client.h"

HttpClient::HttpClient() {
  lastResponse = "";
}

HttpClient::~HttpClient() {
  http.end();
}

uint16_t HttpClient::ping(const String& url, uint16_t timeout) {
  // Add extra safety checks for ping
  if (url.length() > 200) {
    Serial.println("[HTTP] ERROR: URL too long for ping");
    return 0;
  }
  
  // Use shorter timeout for ping to prevent blocking
  if (timeout > 10000) timeout = 10000;
  
  return performRequest(url, timeout, "GET");
}

uint16_t HttpClient::healthCheck(const String& url, const String& endpoint, uint16_t timeout) {
  // Limit URL length to prevent stack overflow
  if (url.length() > 200) {
    Serial.println("[HTTP] ERROR: URL too long for health check");
    return 0;
  }
  
  String fullUrl = url;
  if (endpoint.length() > 0 && endpoint.length() < 100) { // Limit endpoint length
    if (fullUrl.endsWith("/") && endpoint[0] == '/') {
      fullUrl = fullUrl.substring(0, fullUrl.length() - 1);
    } else if (!fullUrl.endsWith("/") && endpoint[0] != '/') {
      fullUrl += "/";
    }
    fullUrl += endpoint;
  }
  
  // Limit total URL length
  if (fullUrl.length() > 300) {
    Serial.println("[HTTP] ERROR: Full URL too long for health check");
    return 0;
  }
  
  uint16_t latency = performRequest(fullUrl, timeout, "GET");
  
  if (latency > 0) {
    // For health checks, also verify the response content
    // Limit response size check to prevent memory issues
    if (lastResponse.length() > 0 && lastResponse.length() < 1000) {
      if (!isHealthyResponse(lastResponse)) {
        return 0; // Consider as failed if response is not healthy
      }
    }
  }
  
  return latency;
}

String HttpClient::get(const String& url, uint16_t timeout) {
  performRequest(url, timeout, "GET");
  return lastResponse;
}

String HttpClient::post(const String& url, const String& data, uint16_t timeout) {
  performRequest(url, timeout, "POST", data);
  return lastResponse;
}

bool HttpClient::isHealthyResponse(const String& response) const {
  if (response.length() == 0) return false;
  
  // Check for explicit unhealthy indicators first
  if (response.indexOf("\"status\":\"unhealthy\"") > 0 || 
      response.indexOf("\"status\":\"down\"") > 0 ||
      response.indexOf("502 Bad Gateway") > 0 ||
      response.indexOf("503 Service Unavailable") > 0 ||
      response.indexOf("504 Gateway Timeout") > 0) {
    return false;
  }
  
  // Check for explicit healthy indicators
  if (response.indexOf("\"status\":\"healthy\"") > 0 ||
      response.indexOf("\"status\":\"ok\"") > 0 ||
      response.indexOf("\"health\":\"ok\"") > 0) {
    return true;
  }
  
  // For responses without explicit health indicators, 
  // only consider healthy if it's a short response (likely a simple OK)
  if (response.length() < 100) {
    return true;
  }
  
  // For longer responses without health indicators, consider unhealthy
  return false;
}

void HttpClient::setUserAgent(const String& userAgent) {
  // This would be implemented if we need custom user agents
}

void HttpClient::addHeader(const String& name, const String& value) {
  // This would be implemented if we need custom headers
}

void HttpClient::clearHeaders() {
  // This would be implemented if we need to clear headers
}

uint16_t HttpClient::performRequest(const String& url, uint16_t timeout, const String& method, const String& data) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[HTTP] WiFi not connected");
    return 0;
  }
  
  // Limit URL length to prevent stack overflow
  if (url.length() > 500) {
    Serial.println("[HTTP] ERROR: URL too long");
    return 0;
  }
  
  // Limit data length
  if (data.length() > 1000) {
    Serial.println("[HTTP] ERROR: Data too long");
    return 0;
  }
  
  // Adjust timeout for slow services
  if (url.indexOf("ngrok-free.app") >= 0 || url.indexOf("trycloudflare.com") >= 0) {
    if (timeout < 7000) timeout = 7000;
  }
  
  // Limit maximum timeout to prevent blocking
  if (timeout > 15000) timeout = 15000;
  
  uint32_t startTime = millis();
  int httpCode = -1;
  
  // Clear last response to free memory
  lastResponse = "";
  
  if (isHttpsUrl(url)) {
    WiFiClientSecure client;
    setupSecureClient(client, url);
    client.setTimeout((timeout + 999) / 1000);
    
    if (http.begin(client, url)) {
      setupHeaders(url);
      if (method == "GET") {
        httpCode = http.GET();
      } else if (method == "POST") {
        http.addHeader("Content-Type", "application/json");
        httpCode = http.POST(data);
      }
      
      if (httpCode > 0) {
        // Limit response size to prevent memory issues
        String response = http.getString();
        if (response.length() > 2000) {
          lastResponse = response.substring(0, 2000) + "...";
          Serial.println("[HTTP] WARNING: Response truncated due to size");
        } else {
          lastResponse = response;
        }
      }
      http.end();
    }
  } else {
    WiFiClient client;
    client.setTimeout((timeout + 999) / 1000);
    
    if (http.begin(client, url)) {
      setupHeaders(url);
      if (method == "GET") {
        httpCode = http.GET();
      } else if (method == "POST") {
        http.addHeader("Content-Type", "application/json");
        httpCode = http.POST(data);
      }
      
      if (httpCode > 0) {
        // Limit response size to prevent memory issues
        String response = http.getString();
        if (response.length() > 2000) {
          lastResponse = response.substring(0, 2000) + "...";
          Serial.println("[HTTP] WARNING: Response truncated due to size");
        } else {
          lastResponse = response;
        }
      }
      http.end();
    }
  }
  
  uint32_t duration = millis() - startTime;
  Serial.printf("[HTTP] %s -> code=%d (%lums)\n", url.c_str(), httpCode, (unsigned long)duration);
  
  if (httpCode > 0 && httpCode != 400) {
    if (duration > 65535) duration = 65535;
    return (uint16_t)duration;
  }
  
  return 0;
}

bool HttpClient::isHttpsUrl(const String& url) const {
  return url.startsWith("https://");
}

void HttpClient::setupSecureClient(WiFiClientSecure& client, const String& url) const {
  client.setInsecure();
  
  if (url.indexOf("ngrok-free.app") >= 0) {
    // Specific settings for ngrok
    client.setCACert(nullptr);
    client.setInsecure();
  }
}

void HttpClient::setupHeaders(const String& url) {
  http.addHeader("User-Agent", "NebulaWatch/1.0");
  http.addHeader("Accept", "*/*");
  
  if (url.indexOf("ngrok-free.app") >= 0) {
    http.addHeader("ngrok-skip-browser-warning", "true");
  }
}
