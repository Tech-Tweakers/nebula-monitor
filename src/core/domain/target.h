#pragma once
#include "core/domain/status.h"
#include <Arduino.h>

class Target {
private:
  String name;
  String url;
  String healthEndpoint;
  MonitorType monitorType;
  Status status;
  uint16_t latency;
  
public:
  // Constructor
  Target(const String& name = "", const String& url = "", 
         const String& healthEndpoint = "", MonitorType type = PING);
  
  // Getters
  String getName() const { return name; }
  String getUrl() const { return url; }
  String getHealthEndpoint() const { return healthEndpoint; }
  MonitorType getMonitorType() const { return monitorType; }
  Status getStatus() const { return status; }
  uint16_t getLatency() const { return latency; }
  
  // Setters
  void setName(const String& n) { name = n; }
  void setUrl(const String& u) { url = u; }
  void setHealthEndpoint(const String& he) { healthEndpoint = he; }
  void setMonitorType(MonitorType mt) { monitorType = mt; }
  void setStatus(Status s) { status = s; }
  void setLatency(uint16_t l) { latency = l; }
  
  // Business logic
  bool isHealthy() const { return status == UP; }
  bool isDown() const { return status == DOWN; }
  bool isUnknown() const { return status == UNKNOWN; }
  
  String getStatusText() const;
  String getLatencyText() const;
  
  // Validation
  bool isValid() const;
  String getFullUrl() const;
};
