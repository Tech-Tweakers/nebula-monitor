#include "touch_handler.h"
#include "core/infrastructure/logger/logger.h"

XPT2046_Touchscreen* TouchHandler::touchscreen = nullptr;
SPIClass* TouchHandler::touchscreenSPI = nullptr;
bool TouchHandler::initialized = false;
unsigned long TouchHandler::lastTouchTime = 0;

bool TouchHandler::initialize() {
  if (initialized) return true;

  Serial_println("[TOUCH] Initializing touch handler...");

  setupSPI();
  setupTouchscreen();

  initialized = true;
  Serial_println("[TOUCH] Touch handler initialized successfully!");

  return true;
}

void TouchHandler::cleanup() {
  if (touchscreen) {
    delete touchscreen;
    touchscreen = nullptr;
  }

  if (touchscreenSPI) {
    delete touchscreenSPI;
    touchscreenSPI = nullptr;
  }

  initialized = false;
}

bool TouchHandler::isTouched() {
  if (!initialized || !touchscreen) return false;

  if (!touchscreen->tirqTouched()) return false;

  // Debounce — descarta toques dentro do intervalo mínimo
  unsigned long now = millis();
  if (now - lastTouchTime < TOUCH_DEBOUNCE_MS) return false;

  TS_Point p = touchscreen->getPoint();
  if (p.z > 20) {
    lastTouchTime = now;
    return true;
  }

  return false;
}

void TouchHandler::getTouchCoordinates(int16_t& x, int16_t& y) {
  if (!initialized || !touchscreen) {
    x = y = 0;
    return;
  }

  int16_t rawX, rawY, z;
  getRawCoordinates(rawX, rawY, z);
  mapRawToScreen(rawX, rawY, x, y);
}

void TouchHandler::getRawCoordinates(int16_t& x, int16_t& y, int16_t& z) {
  if (!initialized || !touchscreen) {
    x = y = z = 0;
    return;
  }

  // Três amostras com delay para o XPT2046 estabilizar entre leituras
  TS_Point p1 = touchscreen->getPoint(); delay(2);
  TS_Point p2 = touchscreen->getPoint(); delay(2);
  TS_Point p3 = touchscreen->getPoint();

  x = (p1.x + p2.x + p3.x) / 3;
  y = (p1.y + p2.y + p3.y) / 3;
  z = (p1.z + p2.z + p3.z) / 3;
}

void TouchHandler::mapRawToScreen(int16_t rawX, int16_t rawY, int16_t& screenX, int16_t& screenY) {
  long nx = map(rawY, RAW_Y_MIN, RAW_Y_MAX, 239, 0);
  long ny = map(rawX, RAW_X_MIN, RAW_X_MAX, 319, 0);

  if (nx < 0) nx = 0; if (nx > 239) nx = 239;
  if (ny < 0) ny = 0; if (ny > 319) ny = 319;

  screenX = (int16_t)nx;
  screenY = (int16_t)ny;
}

void TouchHandler::setupSPI() {
  touchscreenSPI = new SPIClass(HSPI);
  if (!touchscreenSPI) return;

  // Sem begin() — TFT_eSPI já inicializou o HSPI, evita duplicate callback
  Serial_println("[TOUCH] SPI initialized (reusing TFT HSPI)");
}

void TouchHandler::setupTouchscreen() {
  touchscreen = new XPT2046_Touchscreen(T_CS, T_IRQ);
  if (!touchscreen) return;

  touchscreen->begin(*touchscreenSPI);
  touchscreen->setRotation(2);

  Serial_printf("[TOUCH] Touchscreen initialized - CS:%d IRQ:%d\n", T_CS, T_IRQ);
}
