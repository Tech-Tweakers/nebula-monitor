#include "display.hpp"
#include <Arduino.h>
#include <lvgl.h>

// TFT display instance
static TFT_eSPI* tft = nullptr;

// LVGL display buffer
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[240 * 10];  // 10 lines buffer
static lv_color_t buf2[240 * 10];  // 10 lines buffer

// LVGL display driver
static lv_disp_drv_t disp_drv;

// LVGL input driver
static lv_indev_drv_t indev_drv;

// Touch state
static lv_indev_t* touch_indev = nullptr;

bool DisplayManager::begin(uint8_t rotation, int backlight_pin) {
  Serial.println("[DISPLAY] Inicializando Display Manager com LVGL + TFT_eSPI...");
  
  // Create TFT instance
  tft = new TFT_eSPI();
  if (!tft) {
    Serial.println("[DISPLAY] ERRO: Falha ao criar TFT!");
    return false;
  }
  
  Serial.println("[DISPLAY] TFT criado, inicializando...");
  
  // Initialize TFT
  tft->init();
  Serial.println("[DISPLAY] TFT inicializado");
  
  // Set rotation
  tft->setRotation(rotation);
  Serial.printf("[DISPLAY] Rotação definida: %d\n", rotation);
  
  // Set brightness (if backlight pin is provided)
  if (backlight_pin >= 0) {
    pinMode(backlight_pin, OUTPUT);
    digitalWrite(backlight_pin, HIGH);
    Serial.printf("[DISPLAY] Backlight pin %d ativado\n", backlight_pin);
  }
  
  // Initialize LVGL
  lv_init();
  Serial.println("[DISPLAY] LVGL inicializado");
  
  // Initialize display buffer
  lv_disp_draw_buf_init(&draw_buf, buf1, buf2, 240 * 10);
  Serial.println("[DISPLAY] Display buffer inicializado");
  
  // Initialize display driver
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = 240;
  disp_drv.ver_res = 320;
  disp_drv.flush_cb = DisplayManager::flush_cb;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);
  Serial.println("[DISPLAY] Display driver registrado");
  
  // Initialize input driver for touch
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = DisplayManager::touch_read;
  touch_indev = lv_indev_drv_register(&indev_drv);
  Serial.println("[DISPLAY] Input driver registrado");
  
  // Clear screen
  tft->fillScreen(TFT_BLACK);
  Serial.println("[DISPLAY] Display Manager inicializado com sucesso!");
  
  return true;
}

void DisplayManager::end() {
  if (tft) {
    delete tft;
    tft = nullptr;
  }
}

void DisplayManager::clearBuffer() {
  // LVGL handles buffer management
}

void DisplayManager::setPixel(int x, int y, uint16_t color) {
  // Use LVGL for pixel operations
}

void DisplayManager::drawRect(int x, int y, int w, int h, uint16_t color) {
  // Use LVGL for drawing
}

void DisplayManager::fillRect(int x, int y, int w, int h, uint16_t color) {
  // Use LVGL for drawing
}

void DisplayManager::drawText(int x, int y, const char* text, uint16_t color, uint16_t bg_color) {
  // Use LVGL for text
}

void DisplayManager::render() {
  // LVGL handles rendering
  lv_timer_handler();
}

void DisplayManager::markDirty() {
  // LVGL handles dirty marking
}

// LVGL callback functions
void DisplayManager::flush_cb(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  
  tft->startWrite();
  tft->setAddrWindow(area->x1, area->y1, w, h);
  tft->pushColors((uint16_t*)&color_p->full, w * h, true);
  tft->endWrite();
  
  lv_disp_flush_ready(disp);
}

void DisplayManager::touch_read(lv_indev_drv_t* indev, lv_indev_data_t* data) {
  // This will be implemented in touch.cpp
  data->state = LV_INDEV_STATE_REL;
  data->point.x = 0;
  data->point.y = 0;
}

// Convenience functions
bool initDisplay(uint8_t rotation, int backlight_pin) {
  return DisplayManager::begin(rotation, backlight_pin);
}

void renderFrame() {
  DisplayManager::render();
}

void clearScreen() {
  if (tft) {
    tft->fillScreen(TFT_BLACK);
  }
}
