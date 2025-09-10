#pragma once
#include "core/domain/status/status.h"
#include <Arduino.h>

class LEDController {
private:
  // LED pins
  static int LED_PIN_R;
  static int LED_PIN_G;
  static int LED_PIN_B;
  static bool LED_ACTIVE_HIGH;
  
  // PWM configuration
  static const int LEDC_CHANNEL_R = 0;
  static const int LEDC_CHANNEL_G = 1;
  static const int LEDC_CHANNEL_B = 2;
  static int LEDC_FREQ;
  static int LEDC_RES_BITS;
  
  // Brightness levels
  static int LED_BRIGHT_R;
  static int LED_BRIGHT_G;
  static int LED_BRIGHT_B;
  
  // State
  static LEDStatus current_status;
  static int last_led_r;
  static int last_led_g;
  static int last_led_b;
  static bool initialized;
  
  // WiFi blink state
  static unsigned long last_blink;
  static bool blink_state;
  
public:
  // Initialization
  static bool initialize();
  static void cleanup();
  
  // LED control
  static void setStatus(LEDStatus status);
  static void update();
  
  // Individual LED control
  static void setLED(bool r_on, bool g_on, bool b_on);
  static void setRed(bool on);
  static void setGreen(bool on);
  static void setBlue(bool on);
  static void setAllOff();
  
  // Status
  static bool isInitialized() { return initialized; }
  static LEDStatus getCurrentStatus() { return current_status; }
  
private:
  // Internal methods
  static void setupPWM();
  static void updateLEDState();
  static LEDStatus determineStatus();
  static void handleWiFiBlink();
};
