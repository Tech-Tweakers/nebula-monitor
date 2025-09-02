#include "touch.hpp"
#include "config.hpp"
#include <SPI.h>
#include <XPT2046_Touchscreen.h>

// Touchscreen instance
static XPT2046_Touchscreen* touchscreen = nullptr;
static SPIClass* touchscreenSPI = nullptr;

// Touch state
static bool lastTouchState = false;
static uint32_t lastTouchTime = 0;

bool Touch::beginHSPI() {
  Serial.println("[TOUCH] Inicializando touchscreen HSPI...");
  
  // Create SPI instance for touchscreen (HSPI)
  touchscreenSPI = new SPIClass(HSPI);
  if (!touchscreenSPI) {
    Serial.println("[TOUCH] ERRO: Falha ao criar SPI!");
    return false;
  }
  
  // Create touchscreen instance
  touchscreen = new XPT2046_Touchscreen(T_CS, T_IRQ);
  if (!touchscreen) {
    Serial.println("[TOUCH] ERRO: Falha ao criar touchscreen!");
    return false;
  }
  
  // Initialize SPI
  touchscreenSPI->begin(T_SCK, T_MISO, T_MOSI, T_CS);
  
  // Initialize touchscreen
  touchscreen->begin(*touchscreenSPI);
  touchscreen->setRotation(ROT);
  
  Serial.printf("[TOUCH] Touchscreen inicializado - CS:%d IRQ:%d MISO:%d MOSI:%d (HSPI)\n", 
                T_CS, T_IRQ, T_MISO, T_MOSI);
  Serial.println("[TOUCH] Configuração otimizada para máxima sensibilidade!");
  
  return true;
}

bool Touch::touched() {
  if (!touchscreen) return false;
  
  // Check if touchscreen was touched with improved sensitivity
  if (touchscreen->tirqTouched()) {
    // Get touch point to check pressure
    TS_Point p = touchscreen->getPoint();
    
    // Very low pressure threshold for maximum sensitivity
    if (p.z > 20) { // Ultra-low pressure threshold
      return true;
    }
  }
  
  return false;
}

void Touch::readRaw(int16_t& x, int16_t& y, int16_t& z) {
  if (!touchscreen) {
    x = y = z = 0;
    return;
  }
  
  // Get touch point with averaging for better accuracy
  TS_Point p1 = touchscreen->getPoint();
  TS_Point p2 = touchscreen->getPoint();
  TS_Point p3 = touchscreen->getPoint();
  
  // Average multiple readings for better accuracy
  x = (p1.x + p2.x + p3.x) / 3;
  y = (p1.y + p2.y + p3.y) / 3;
  z = (p1.z + p2.z + p3.z) / 3;
  
  // Debug pressure for sensitivity tuning
  TOUCH_LOGF("[TOUCH] Pressure: %d (threshold: 10)\n", z);
}

void Touch::mapRawToScreen(int16_t rx, int16_t ry, int& sx, int& sy) {
  // Map raw touch coordinates to screen coordinates
  // Using the calibration values from the CYD tutorial
  // For rotation 2 (landscape), we need to swap and invert coordinates
  
  // Debug: print raw values
  TOUCH_LOGF("[TOUCH] Raw: x=%d, y=%d\n", rx, ry);
  
  // For rotation 2 (landscape), swap X and Y and invert Y
  // Also invert X to fix mirroring
  long nx = map(ry, RAW_Y_MIN, RAW_Y_MAX, 239, 0); // Y becomes X, inverted (240 width)
  long ny = map(rx, RAW_X_MIN, RAW_X_MAX, 319, 0); // X becomes Y, inverted (320 height)
  
  // Clamp to screen bounds
  if (nx < 0) nx = 0; if (nx > 239) nx = 239;
  if (ny < 0) ny = 0; if (ny > 319) ny = 319;
  
  sx = (int)nx;
  sy = (int)ny;
  
  // Debug: print mapped values
  TOUCH_LOGF("[TOUCH] Mapped: x=%d, y=%d\n", sx, sy);
}
