#pragma once
#include <TFT_eSPI.h>
#include <lvgl.h>
#include "config.hpp"

class DisplayManager {
private:
  static bool bufferDirty;
  static int bufferWidth;
  static int bufferHeight;
  
public:
  static bool begin(uint8_t rotation = ROT, int backlight_pin = BL_PIN);
  static void end();
  static void clearBuffer();
  static void setPixel(int x, int y, uint16_t color);
  static void drawRect(int x, int y, int w, int h, uint16_t color);
  static void fillRect(int x, int y, int w, int h, uint16_t color);
  static void drawText(int x, int y, const char* text, uint16_t color, uint16_t bg_color);
  static void render();
  static void markDirty();
  static int width() { return bufferWidth; }
  static int height() { return bufferHeight; }
  static bool isInitialized() { return true; }
  
  // LVGL callback functions
  static void flush_cb(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p);
  static void touch_read(lv_indev_drv_t* indev, lv_indev_data_t* data);
};

// Funções de conveniência
bool initDisplay(uint8_t rotation = ROT, int backlight_pin = BL_PIN);
void renderFrame();
void clearScreen();
