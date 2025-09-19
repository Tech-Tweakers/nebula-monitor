#include "core/infrastructure/http_client/http_client.h"
#include "config/config_loader/config_loader.h"
#include "core/infrastructure/memory_manager/memory_manager.h"
#include "core/infrastructure/logger/logger.h"

HttpClient::HttpClient() {
  lastResponse = "";
  lastHttpCode = -1;
  secureClient = nullptr;
  plainClient = nullptr;
  clientInitialized = false;
  
  // Initialize metrics
  metrics.totalRequests = 0;
  metrics.successfulRequests = 0;
  metrics.sslErrors = 0;
  metrics.timeoutErrors = 0;
  metrics.lastErrorTime = 0;
  metrics.lastErrorCategory = ErrorCategory::UNKNOWN;
  
  // Initialize intelligent logging
  metrics.lastLogTime = 0;
  metrics.errorCountSinceLastLog = 0;
  metrics.suppressRepeatedErrors = false;
  
  initializeClients();
}

HttpClient::~HttpClient() {
  http.end();
  cleanupClients();
}

uint16_t HttpClient::ping(const String& url, uint16_t timeout) {
  // Enhanced safety checks
  if (url.length() > 200) {
    Serial_println("[HTTP] ERROR: URL too long for ping");
    return 0;
  }
  
  // Calculate intelligent timeout
  uint16_t calculatedTimeout = calculateTimeout(url, timeout);
  
  return performRequestWithRetry(url, calculatedTimeout, "GET");
}

uint16_t HttpClient::healthCheck(const String& url, const String& endpoint, uint16_t timeout) {
  // Enhanced safety checks
  if (url.length() > 200) {
    Serial_println("[HTTP] ERROR: URL too long for health check");
    return 0;
  }
  
  // Build full URL efficiently
  String fullUrl = url;
  if (endpoint.length() > 0 && endpoint.length() < 100) {
    if (fullUrl.endsWith("/") && endpoint[0] == '/') {
      fullUrl = fullUrl.substring(0, fullUrl.length() - 1);
    } else if (!fullUrl.endsWith("/") && endpoint[0] != '/') {
      fullUrl += "/";
    }
    fullUrl += endpoint;
  }
  
  if (fullUrl.length() > 300) {
    Serial_println("[HTTP] ERROR: Full URL too long for health check");
    return 0;
  }
  
  // Calculate intelligent timeout for health checks
  uint16_t calculatedTimeout = calculateTimeout(fullUrl, timeout);
  
  uint16_t latency = performRequestWithRetry(fullUrl, calculatedTimeout, "GET");
  
  if (latency > 0) {
    // Check HTTP status code first
    if (lastHttpCode < 200 || lastHttpCode >= 300) {
      Serial_printf("[HTTP] Health check failed: HTTP %d\n", lastHttpCode);
      return 0;
    }
    
    // Enhanced health response validation
    if (lastResponse.length() > 0 && lastResponse.length() < 1000) {
      if (!isHealthyResponse(lastResponse)) {
        Serial_printf("[HTTP] Health check failed: Unhealthy response detected (HTTP %d)\n", lastHttpCode);
        return 0;
      }
    }
    
    // Log successful health check with details
    Serial_printf("[HTTP] Health check successful: %d ms (HTTP %d)\n", latency, lastHttpCode);
  }
  
  return latency;
}

String HttpClient::get(const String& url, uint16_t timeout) {
  uint16_t calculatedTimeout = calculateTimeout(url, timeout);
  performRequestWithRetry(url, calculatedTimeout, "GET");
  return lastResponse;
}

String HttpClient::post(const String& url, const String& data, uint16_t timeout) {
  uint16_t calculatedTimeout = calculateTimeout(url, timeout);
  performRequestWithRetry(url, calculatedTimeout, "POST", data);
  return lastResponse;
}

bool HttpClient::isHealthyResponse(const String& response) const {
  if (response.length() == 0) return false;
  
  // Convert to lowercase for case-insensitive comparison
  String lowerResponse = response;
  lowerResponse.toLowerCase();
  
  // Get configuration patterns
  String unhealthyPatterns = ConfigLoader::getHealthCheckUnhealthyPatterns();
  String healthyPatterns = ConfigLoader::getHealthCheckHealthyPatterns();
  bool strictMode = ConfigLoader::isHealthCheckStrictMode();
  
  // Check for explicit unhealthy indicators first
  int start = 0;
  while (start < unhealthyPatterns.length()) {
    int end = unhealthyPatterns.indexOf(',', start);
    if (end == -1) end = unhealthyPatterns.length();
    
    String pattern = unhealthyPatterns.substring(start, end);
    pattern.trim();
    if (pattern.length() > 0 && lowerResponse.indexOf(pattern) >= 0) {
      return false;
    }
    start = end + 1;
  }
  
  // Check for explicit healthy indicators
  start = 0;
  while (start < healthyPatterns.length()) {
    int end = healthyPatterns.indexOf(',', start);
    if (end == -1) end = healthyPatterns.length();
    
    String pattern = healthyPatterns.substring(start, end);
    pattern.trim();
    if (pattern.length() > 0 && lowerResponse.indexOf(pattern) >= 0) {
      return true;
    }
    start = end + 1;
  }
  
  // Check for simple success responses
  if (lowerResponse == "ok" || 
      lowerResponse == "healthy" || 
      lowerResponse == "up" ||
      lowerResponse == "running" ||
      lowerResponse == "{\"ok\":true}" ||
      lowerResponse == "{\"status\":\"ok\"}" ||
      lowerResponse == "{\"health\":\"ok\"}") {
    return true;
  }
  
  // In strict mode, require explicit health indicators
  if (strictMode) {
    return false;
  }
  
  // For responses without explicit health indicators, 
  // consider healthy if it's a short response (likely a simple status)
  if (response.length() < 200) {
    // Additional check: if it contains JSON-like structure, be more strict
    if (response.indexOf("{") >= 0 && response.indexOf("}") >= 0) {
      // It's JSON but doesn't have clear health indicators, consider unhealthy
      return false;
    }
    // Simple text response, consider healthy
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
    Serial_println("[HTTP] WiFi not connected");
    return 0;
  }
  
  // Check memory before proceeding
  if (MemoryManager::getInstance().isMemoryCritical()) {
    Serial_println("[HTTP] ERROR: Critical memory condition, skipping request");
    return 0;
  }
  
  // Limit URL length to prevent stack overflow
  if (url.length() > 500) {
    Serial_println("[HTTP] ERROR: URL too long");
    return 0;
  }
  
  // Limit data length
  if (data.length() > 1000) {
    Serial_println("[HTTP] ERROR: Data too long");
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
  lastResponse.reserve(0); // Force memory deallocation
  lastHttpCode = -1;
  
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
          Serial_println("[HTTP] WARNING: Response truncated due to size");
        } else {
          lastResponse = response;
        }
        
        // Force cleanup of temporary response
        response = "";
        response.reserve(0);
      }
      http.end();
    }
    
    // CRITICAL FIX: Force SSL context cleanup to prevent stack overflow
    // This is essential when endpoints are offline and SSL handshake fails
    client.stop();
    client.flush();
    
    // Additional cleanup for failed SSL connections
    if (httpCode == -1) {
      Serial_println("[HTTP] SSL connection failed, forcing cleanup...");
      
      // CRITICAL: Force delay to allow SSL cleanup to complete
      // This prevents stack accumulation during rapid retries
      delay(100);
    }
    
    // CRITICAL: Always force cleanup after SSL operations
    // This prevents the stack canary watchpoint from triggering
    delay(50);
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
          Serial_println("[HTTP] WARNING: Response truncated due to size");
        } else {
          lastResponse = response;
        }
        
        // Force cleanup of temporary response
        response = "";
        response.reserve(0);
      }
      http.end();
    }
  }
  
  // Save the HTTP code for later retrieval
  lastHttpCode = httpCode;
  
  uint32_t duration = millis() - startTime;
  Serial_printf("[HTTP] %s -> code=%d (%lums)\n", url.c_str(), httpCode, (unsigned long)duration);
  
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
  
  // CRITICAL FIX: Set shorter timeout to prevent SSL context accumulation
  // This prevents SSL handshake from hanging and accumulating resources
  client.setTimeout(5000); // 5 second timeout for SSL handshake
}

void HttpClient::setupHeaders(const String& url) {
  http.addHeader("User-Agent", "NebulaWatch/1.0");
  http.addHeader("Accept", "*/*");
  
  if (url.indexOf("ngrok-free.app") >= 0) {
    http.addHeader("ngrok-skip-browser-warning", "true");
  }
}

// ===== ENHANCED IMPLEMENTATION =====

void HttpClient::initializeClients() {
  if (!clientInitialized) {
    secureClient = new WiFiClientSecure();
    plainClient = new WiFiClient();
    clientInitialized = true;
  }
}

void HttpClient::cleanupClients() {
  if (clientInitialized) {
    delete secureClient;
    delete plainClient;
    secureClient = nullptr;
    plainClient = nullptr;
    clientInitialized = false;
  }
}

bool HttpClient::isConnectionHealthy() const {
  return WiFi.status() == WL_CONNECTED;
}

uint16_t HttpClient::calculateTimeout(const String& url, uint16_t requestedTimeout) const {
  if (requestedTimeout > 0) {
    return min(requestedTimeout, (uint16_t)8000); // Cap at 8s (reduced from 15s)
  }
  
  ConnectionConfig config = getConnectionConfig(url);
  return min(config.baseTimeout, (uint16_t)8000); // Cap all timeouts at 8s
}

ConnectionConfig HttpClient::getConnectionConfig(const String& url) const {
  ConnectionConfig config;
  
  if (url.indexOf("ngrok-free.app") >= 0) {
    config.baseTimeout = 8000;
    config.maxTimeout = 12000;
    config.useInsecure = true;
    config.retryOnError = true;
    config.maxRetries = 2;
    config.userAgent = "NebulaWatch/1.0";
  } else if (url.indexOf("trycloudflare.com") >= 0) {
    config.baseTimeout = 7000;
    config.maxTimeout = 10000;
    config.useInsecure = true;
    config.retryOnError = true;
    config.maxRetries = 2;
    config.userAgent = "NebulaWatch/1.0";
  } else if (url.indexOf("github.io") >= 0) {
    config.baseTimeout = 5000;
    config.maxTimeout = 8000;
    config.useInsecure = false;
    config.retryOnError = true;
    config.maxRetries = 1;
    config.userAgent = "NebulaWatch/1.0";
  } else {
    // Default configuration
    config.baseTimeout = 5000;
    config.maxTimeout = 10000;
    config.useInsecure = false;
    config.retryOnError = true;
    config.maxRetries = 1;
    config.userAgent = "NebulaWatch/1.0";
  }
  
  return config;
}

ErrorCategory HttpClient::categorizeError(int httpCode, const String& url) const {
  if (httpCode == -1) {
    // Connection failed - check if it's SSL related
    if (url.startsWith("https://")) {
      return ErrorCategory::SSL_ERROR; // HTTPS connection failures are likely SSL
    }
    if (url.indexOf("ngrok") >= 0 || url.indexOf("cloudflare") >= 0) {
      return ErrorCategory::TEMPORARY; // Tunnel services are often temporary
    }
    return ErrorCategory::TEMPORARY;
  }
  
  if (httpCode >= 500) {
    return ErrorCategory::TEMPORARY; // Server errors are usually temporary
  }
  
  if (httpCode >= 400 && httpCode < 500) {
    return ErrorCategory::PERMANENT; // Client errors are usually permanent
  }
  
  return ErrorCategory::UNKNOWN;
}

bool HttpClient::shouldRetry(ErrorCategory category, uint8_t retryCount) const {
  if (category == ErrorCategory::PERMANENT) {
    return false; // Don't retry permanent errors
  }
  
  if (category == ErrorCategory::SSL_ERROR && retryCount < 1) {
    return true; // Retry SSL errors once (they're often temporary)
  }
  
  if (category == ErrorCategory::TEMPORARY && retryCount < 2) {
    return true; // Retry temporary errors up to 2 times
  }
  
  return false;
}

uint16_t HttpClient::performRequestWithRetry(const String& url, uint16_t timeout, const String& method, const String& data) {
  ConnectionConfig config = getConnectionConfig(url);
  uint8_t retryCount = 0;
  uint16_t lastLatency = 0;
  
  while (retryCount <= config.maxRetries) {
    // Feed watchdog before each request attempt
    MemoryManager::getInstance().feedWatchdog();
    
    lastLatency = performRequest(url, timeout, method, data);
    
    // Feed watchdog after each request attempt
    MemoryManager::getInstance().feedWatchdog();
    
    if (lastLatency > 0) {
      // Success
      metrics.successfulRequests++;
      return lastLatency;
    }
    
    // Failed - categorize error
    ErrorCategory errorCategory = categorizeError(-1, url); // -1 indicates connection failure
    metrics.lastErrorCategory = errorCategory;
    metrics.lastErrorTime = millis();
    
    if (errorCategory == ErrorCategory::SSL_ERROR) {
      metrics.sslErrors++;
    } else if (errorCategory == ErrorCategory::TEMPORARY) {
      metrics.timeoutErrors++;
    }
    
    if (!shouldRetry(errorCategory, retryCount)) {
      break;
    }
    
    retryCount++;
    if (retryCount <= config.maxRetries) {
      Serial_printf("[HTTP] Retry %d/%d for %s\n", retryCount, config.maxRetries, url.c_str());
      
      // CRITICAL FIX: Force cleanup between retries to prevent SSL context accumulation
      if (isHttpsUrl(url)) {
        Serial_println("[HTTP] Forcing SSL cleanup between retries...");
        
        // CRITICAL: Force delay to allow SSL cleanup to complete
        // This prevents stack canary watchpoint from triggering
        delay(200);
      }
      
      vTaskDelay(pdMS_TO_TICKS(1000 * retryCount)); // Exponential backoff
    }
  }
  
  metrics.totalRequests++;
  return 0;
}

void HttpClient::printMetrics() const {
  Serial_println("\n=== HTTP CLIENT METRICS ===");
  Serial_printf("Total Requests: %lu\n", metrics.totalRequests);
  Serial_printf("Successful: %lu\n", metrics.successfulRequests);
  Serial_printf("SSL Errors: %lu\n", metrics.sslErrors);
  Serial_printf("Timeout Errors: %lu\n", metrics.timeoutErrors);
  Serial_printf("Success Rate: %.1f%%\n", getSuccessRate());
  Serial_printf("Last Error Category: %d\n", (int)metrics.lastErrorCategory);
  Serial_println("========================\n");
}

void HttpClient::resetMetrics() {
  metrics.totalRequests = 0;
  metrics.successfulRequests = 0;
  metrics.sslErrors = 0;
  metrics.timeoutErrors = 0;
  metrics.lastErrorTime = 0;
  metrics.lastErrorCategory = ErrorCategory::UNKNOWN;
  
  // Reset intelligent logging
  metrics.lastLogTime = 0;
  metrics.errorCountSinceLastLog = 0;
  metrics.suppressRepeatedErrors = false;
}

float HttpClient::getSuccessRate() const {
  if (metrics.totalRequests == 0) return 100.0f;
  return (float)metrics.successfulRequests / metrics.totalRequests * 100.0f;
}

void HttpClient::logErrorIntelligently(const String& message, ErrorCategory category) {
  uint32_t currentTime = millis();
  
  // Check if we should log this error
  if (!shouldLogError(category)) {
    metrics.errorCountSinceLastLog++;
    return;
  }
  
  // Reset suppression if it's been more than 30 seconds
  if ((currentTime - metrics.lastLogTime) >= 30000) {
    metrics.suppressRepeatedErrors = false;
  }
  
  // Log the error
  Serial_printf("[HTTP] %s", message.c_str());
  
  // If we've been suppressing errors, show the count
  if (metrics.errorCountSinceLastLog > 0) {
    Serial_printf(" (suppressed %lu similar errors)", metrics.errorCountSinceLastLog);
  }
  Serial_println();
  
  // Update logging state
  metrics.lastLogTime = currentTime;
  metrics.errorCountSinceLastLog = 0;
  metrics.suppressRepeatedErrors = true;
}

bool HttpClient::shouldLogError(ErrorCategory category) const {
  uint32_t currentTime = millis();
  
  // Always log the first error
  if (metrics.lastLogTime == 0) {
    return true;
  }
  
  // Don't log if we're suppressing repeated errors and it's been less than 30 seconds
  if (metrics.suppressRepeatedErrors && (currentTime - metrics.lastLogTime) < 30000) {
    return false;
  }
  
  // Reset suppression after 30 seconds
  if ((currentTime - metrics.lastLogTime) >= 30000) {
    return true;
  }
  
  return true;
}
