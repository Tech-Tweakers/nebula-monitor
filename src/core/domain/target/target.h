#pragma once
#include "core/domain/status/status.h"
#include <Arduino.h>

class Target {
private:
  String name;
  String url;
  String healthEndpoint;
  String group;
  MonitorType monitorType;
  Status status;
  uint16_t latency;

  // Session stats
  uint16_t failCount;
  unsigned long lastDownTime;
  unsigned long lastDownDuration;
  unsigned long lastStatusChange;
  
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
  void setGroup(const String& g) { group = g.length() > 0 ? g : "Default"; }
  void setMonitorType(MonitorType mt) { monitorType = mt; }
  String getGroup() const { return group; }
  void setLatency(uint16_t l) { latency = l; }
  void setStatus(Status s) {
    if (s != status) {
      lastStatusChange = millis();
      if (s == DOWN) lastDownTime = millis();
      if (status == DOWN && s != DOWN) lastDownDuration = millis() - lastDownTime;
      if (s == DOWN) failCount++;
    }
    status = s;
  }

  // Session stats getters
  uint16_t getFailCount() const { return failCount; }
  unsigned long getLastDownDuration() const { return lastDownDuration; }
  unsigned long getLastStatusChange() const { return lastStatusChange; }
  
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
