#pragma once
#include <Arduino.h>

// Status enumeration for network targets
enum Status : uint8_t { 
  UNKNOWN = 0, 
  UP = 1, 
  DOWN = 2 
};

// Monitor type enumeration
enum MonitorType : uint8_t { 
  PING = 0, 
  HEALTH_CHECK = 1 
};

// LED Status States - Priority order (higher number = higher priority)
enum class LEDStatus {
  OFF = 0,           // All LEDs off
  SYSTEM_OK = 1,     // Green LED - System idle/OK
  TARGETS_DOWN = 2,  // Blue LED - Active alerts (targets down)
  SCANNING = 3,      // Red LED - Scanning in progress
  TELEGRAM = 4,      // Red LED - Telegram sending
  WIFI_DISCONNECTED = 5  // Red LED blinking - WiFi disconnected
};

// Scan event types for inter-task communication
enum ScanEventType { 
  EV_SCAN_START, 
  EV_SCAN_COMPLETE, 
  EV_TARGET_UPDATE 
};

// Scan event structure
struct ScanEvent {
  ScanEventType type;
  int index;           // used for EV_TARGET_UPDATE
  Status status;       // used for EV_TARGET_UPDATE
  uint16_t latency_ms; // used for EV_TARGET_UPDATE
};
