#pragma once
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Arduino.h>

// Error categories for intelligent handling
enum class ErrorCategory {
  TEMPORARY,    // Network issues, timeouts
  PERMANENT,    // SSL cert issues, 404, etc
  SSL_ERROR,    // SSL/TLS specific errors
  UNKNOWN       // Default category
};

// Connection configuration for different service types
struct ConnectionConfig {
  uint16_t baseTimeout;
  uint16_t maxTimeout;
  bool useInsecure;
  bool retryOnError;
  uint8_t maxRetries;
  const char* userAgent;
};

class HttpClient {
private:
  HTTPClient http;
  String lastResponse;
  int lastHttpCode;
  
  // Performance metrics
  struct Metrics {
    uint32_t totalRequests;
    uint32_t successfulRequests;
    uint32_t sslErrors;
    uint32_t timeoutErrors;
    uint32_t lastErrorTime;
    ErrorCategory lastErrorCategory;
    
    // Intelligent logging
    uint32_t lastLogTime;
    uint32_t errorCountSinceLastLog;
    bool suppressRepeatedErrors;
  } metrics;
  
  // Connection pooling (simple implementation)
  WiFiClientSecure* secureClient;
  WiFiClient* plainClient;
  bool clientInitialized;
  
public:
  HttpClient();
  ~HttpClient();
  
  // Basic HTTP operations with enhanced error handling
  uint16_t ping(const String& url, uint16_t timeout = 0);
  uint16_t healthCheck(const String& url, const String& endpoint, uint16_t timeout = 0);
  String get(const String& url, uint16_t timeout = 0);
  String post(const String& url, const String& data, uint16_t timeout = 0);
  
  // Response handling
  String getLastResponse() const { return lastResponse; }
  int getLastHttpCode() const { return lastHttpCode; }
  bool isHealthyResponse(const String& response) const;
  
  // Enhanced utility methods
  void setUserAgent(const String& userAgent);
  void addHeader(const String& name, const String& value);
  void clearHeaders();
  
  // Performance and diagnostics
  void printMetrics() const;
  void resetMetrics();
  float getSuccessRate() const;
  ErrorCategory getLastErrorCategory() const { return metrics.lastErrorCategory; }
  
private:
  // Core request handling with retry logic
  uint16_t performRequest(const String& url, uint16_t timeout, const String& method = "GET", const String& data = "");
  uint16_t performRequestWithRetry(const String& url, uint16_t timeout, const String& method = "GET", const String& data = "");
  
  // Enhanced SSL/TLS handling
  bool isHttpsUrl(const String& url) const;
  void setupSecureClient(WiFiClientSecure& client, const String& url) const;
  void setupHeaders(const String& url);
  
  // Intelligent timeout calculation
  uint16_t calculateTimeout(const String& url, uint16_t requestedTimeout) const;
  ConnectionConfig getConnectionConfig(const String& url) const;
  
  // Error handling and categorization
  ErrorCategory categorizeError(int httpCode, const String& url) const;
  bool shouldRetry(ErrorCategory category, uint8_t retryCount) const;
  
  // Intelligent logging
  void logErrorIntelligently(const String& message, ErrorCategory category);
  bool shouldLogError(ErrorCategory category) const;
  
  // Connection management
  void initializeClients();
  void cleanupClients();
  bool isConnectionHealthy() const;
};
