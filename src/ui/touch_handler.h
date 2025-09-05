#pragma once
#include <Arduino.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>

class TouchHandler {
private:
  static XPT2046_Touchscreen* touchscreen;
  static SPIClass* touchscreenSPI;
  static bool initialized;
  
  // Touch calibration
  static const int RAW_X_MIN = 200;
  static const int RAW_X_MAX = 3700;
  static const int RAW_Y_MIN = 240;
  static const int RAW_Y_MAX = 3800;
  
  // Pin definitions
  static const int T_SCK = 14;
  static const int T_MISO = 12;
  static const int T_MOSI = 13;
  static const int T_CS = 33;
  static const int T_IRQ = 36;
  
public:
  // Initialization
  static bool initialize();
  static void cleanup();
  
  // Touch detection
  static bool isTouched();
  static void getTouchCoordinates(int16_t& x, int16_t& y);
  static void getRawCoordinates(int16_t& x, int16_t& y, int16_t& z);
  
  // Coordinate mapping
  static void mapRawToScreen(int16_t rawX, int16_t rawY, int16_t& screenX, int16_t& screenY);
  
  // Status
  static bool isInitialized() { return initialized; }
  
private:
  // Internal methods
  static void setupSPI();
  static void setupTouchscreen();
};
