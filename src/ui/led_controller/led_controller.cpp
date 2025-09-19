#include "ui/led_controller/led_controller.h"
#include "config/config_loader/config_loader.h"
#include <WiFi.h>
#include "core/infrastructure/logger/logger.h"

// Static member definitions
int LEDController::LED_PIN_R = 16;
int LEDController::LED_PIN_G = 17;
int LEDController::LED_PIN_B = 20;
bool LEDController::LED_ACTIVE_HIGH = false;
int LEDController::LEDC_FREQ = 5000;
int LEDController::LEDC_RES_BITS = 8;
int LEDController::LED_BRIGHT_R = 32;
int LEDController::LED_BRIGHT_G = 12;
int LEDController::LED_BRIGHT_B = 12;
LEDStatus LEDController::current_status = LEDStatus::OFF;
int LEDController::last_led_r = -1;
int LEDController::last_led_g = -1;
int LEDController::last_led_b = -1;
bool LEDController::initialized = false;
unsigned long LEDController::last_blink = 0;
bool LEDController::blink_state = false;

bool LEDController::initialize() {
  if (initialized) return true;
  
  Serial_println("[LED] Initializing LED controller...");
  
  // Load configuration
  LED_PIN_R = ConfigLoader::getLedPinR();
  LED_PIN_G = ConfigLoader::getLedPinG();
  LED_PIN_B = ConfigLoader::getLedPinB();
  LED_ACTIVE_HIGH = ConfigLoader::isLedActiveHigh();
  LEDC_FREQ = ConfigLoader::getLedPwmFreq();
  LEDC_RES_BITS = ConfigLoader::getLedPwmResBits();
  LED_BRIGHT_R = ConfigLoader::getLedBrightR();
  LED_BRIGHT_G = ConfigLoader::getLedBrightG();
  LED_BRIGHT_B = ConfigLoader::getLedBrightB();
  
  // Setup PWM
  setupPWM();
  
  // Start with all LEDs off
  setAllOff();
  
  initialized = true;
  Serial_println("[LED] LED controller initialized successfully!");
  
  return true;
}

void LEDController::cleanup() {
  if (initialized) {
    setAllOff();
    initialized = false;
  }
}

void LEDController::setStatus(LEDStatus status) {
  current_status = status;
  updateLEDState();
}

void LEDController::update() {
  if (!initialized) return;
  
  // Handle WiFi disconnected blinking
  if (WiFi.status() != WL_CONNECTED) {
    handleWiFiBlink();
    return;
  }
  
  // Update LED based on current status
  updateLEDState();
}

void LEDController::setLED(bool r_on, bool g_on, bool b_on) {
  if (!initialized) return;
  
  // Calculate target brightness per channel
  const int r_brightness = r_on ? LED_BRIGHT_R : 0;
  const int g_brightness = g_on ? LED_BRIGHT_G : 0;
  const int b_brightness = b_on ? LED_BRIGHT_B : 0;
  
  // Avoid redundant writes
  if (last_led_r == r_brightness && last_led_g == g_brightness && last_led_b == b_brightness) {
    return;
  }
  
  last_led_r = r_brightness;
  last_led_g = g_brightness;
  last_led_b = b_brightness;
  
  // Invert for common anode if needed
  const int r_duty = LED_ACTIVE_HIGH ? r_brightness : (255 - r_brightness);
  const int g_duty = LED_ACTIVE_HIGH ? g_brightness : (255 - g_brightness);
  const int b_duty = LED_ACTIVE_HIGH ? b_brightness : (255 - b_brightness);
  
  // Apply PWM
  ledcWrite(LEDC_CHANNEL_R, r_duty);
  ledcWrite(LEDC_CHANNEL_G, g_duty);
  ledcWrite(LEDC_CHANNEL_B, b_duty);
}

void LEDController::setRed(bool on) {
  setLED(on, false, false);
}

void LEDController::setGreen(bool on) {
  setLED(false, on, false);
}

void LEDController::setBlue(bool on) {
  setLED(false, false, on);
}

void LEDController::setAllOff() {
  setLED(false, false, false);
}

void LEDController::setupPWM() {
  // Setup PWM channels
  ledcSetup(LEDC_CHANNEL_R, LEDC_FREQ, LEDC_RES_BITS);
  ledcSetup(LEDC_CHANNEL_G, LEDC_FREQ, LEDC_RES_BITS);
  ledcSetup(LEDC_CHANNEL_B, LEDC_FREQ, LEDC_RES_BITS);
  
  // Attach pins
  ledcAttachPin(LED_PIN_R, LEDC_CHANNEL_R);
  ledcAttachPin(LED_PIN_G, LEDC_CHANNEL_G);
  ledcAttachPin(LED_PIN_B, LEDC_CHANNEL_B);
  
  Serial_printf("[LED] PWM setup: R=%d, G=%d, B=%d, Freq=%dHz, Res=%d bits\n", 
               LED_PIN_R, LED_PIN_G, LED_PIN_B, LEDC_FREQ, LEDC_RES_BITS);
}

void LEDController::updateLEDState() {
  switch (current_status) {
    case LEDStatus::OFF:
      setAllOff();
      break;
      
    case LEDStatus::SYSTEM_OK:
      setGreen(true);
      break;
      
    case LEDStatus::TARGETS_DOWN:
      setBlue(true);
      break;
      
    case LEDStatus::SCANNING:
    case LEDStatus::TELEGRAM:
      setRed(true);
      break;
      
    case LEDStatus::WIFI_DISCONNECTED:
      // This will be handled by handleWiFiBlink()
      setRed(true);
      break;
  }
}

LEDStatus LEDController::determineStatus() {
  // Check WiFi status first (highest priority)
  if (WiFi.status() != WL_CONNECTED) {
    return LEDStatus::WIFI_DISCONNECTED;
  }
  
  // Check if Telegram is sending (high priority)
  // This would need access to TelegramService
  // if (TelegramService::isSendingMessage()) {
  //   return LEDStatus::TELEGRAM;
  // }
  
  // Check if scanning is active (high priority)
  // This would need access to NetworkMonitor
  // if (NetworkMonitor::isScanning()) {
  //   return LEDStatus::SCANNING;
  // }
  
  // Check if there are active alerts (targets down)
  // This would need access to TelegramService
  // if (TelegramService::hasActiveAlerts()) {
  //   return LEDStatus::TARGETS_DOWN;
  // }
  
  // System is OK
  return LEDStatus::SYSTEM_OK;
}

void LEDController::handleWiFiBlink() {
  unsigned long now = millis();
  if (now - last_blink >= 500) { // 500ms blink interval
    blink_state = !blink_state;
    last_blink = now;
    
    if (blink_state) {
      setRed(true);  // Red LED on
    } else {
      setAllOff();   // All LEDs off
    }
  }
}
