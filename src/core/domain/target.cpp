#include "target.h"

Target::Target(const String& name, const String& url, 
               const String& healthEndpoint, MonitorType type) 
  : name(name), url(url), healthEndpoint(healthEndpoint), 
    monitorType(type), status(UNKNOWN), latency(0) {
}

String Target::getStatusText() const {
  switch (status) {
    case UP: return "UP";
    case DOWN: return "DOWN";
    case UNKNOWN: return "UNKNOWN";
    default: return "ERROR";
  }
}

String Target::getLatencyText() const {
  if (status != UP || latency == 0) {
    return monitorType == HEALTH_CHECK ? "FAIL" : "DOWN";
  }
  
  String text = String(latency);
  text += monitorType == HEALTH_CHECK ? " OK" : " ms";
  return text;
}

bool Target::isValid() const {
  return name.length() > 0 && url.length() > 0;
}

String Target::getFullUrl() const {
  if (healthEndpoint.length() == 0) {
    return url;
  }
  
  String fullUrl = url;
  if (fullUrl.endsWith("/") && healthEndpoint[0] == '/') {
    fullUrl = fullUrl.substring(0, fullUrl.length() - 1);
  } else if (!fullUrl.endsWith("/") && healthEndpoint[0] != '/') {
    fullUrl += "/";
  }
  fullUrl += healthEndpoint;
  
  return fullUrl;
}
