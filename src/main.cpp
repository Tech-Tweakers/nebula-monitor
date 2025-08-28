#include <Arduino.h>
#include "config.hpp"
#include "display.hpp"
#include "touch.hpp"
#include "net.hpp"
#include "scan.hpp"
#include <lvgl.h>

// Network targets
const Target targets[] = {
  {"Google", "https://www.google.com", UNKNOWN, 0},
  {"GitHub", "https://github.com", UNKNOWN, 0},
  {"Stack Overflow", "https://stackoverflow.com", UNKNOWN, 0},
  {"Reddit", "https://reddit.com", UNKNOWN, 0},
  {"Wikipedia", "https://wikipedia.org", UNKNOWN, 0},
  {"YouTube", "https://youtube.com", UNKNOWN, 0}
};

const int N_TARGETS = sizeof(targets) / sizeof(targets[0]);

// LVGL objects
static lv_obj_t* main_screen;
static lv_obj_t* title_label;
static lv_obj_t* tile_containers[6]; // Fixed size for now

void setup() {
  Serial.begin(115200);
  Serial.println("[MAIN] Iniciando Nebula Monitor v3.0...");

  // Initialize display
  if (!DisplayManager::begin()) {
    Serial.println("[MAIN] ERRO: Falha ao inicializar display!");
    return;
  }
  Serial.println("[MAIN] Display inicializado com sucesso!");

  // Initialize touch
  if (!Touch::beginHSPI()) {
    Serial.println("[MAIN] ERRO: Falha ao inicializar touch!");
    return;
  }
  Serial.println("[MAIN] Touch inicializado com sucesso!");

  // Initialize LVGL screen
  main_screen = lv_scr_act();
  lv_obj_set_style_bg_color(main_screen, lv_color_hex(0x000000), LV_PART_MAIN);

  // Create title bar
  lv_obj_t* title_bar = lv_obj_create(main_screen);
  lv_obj_set_size(title_bar, 240, 30);
  lv_obj_set_pos(title_bar, 0, 0);
  lv_obj_set_style_bg_color(title_bar, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_border_width(title_bar, 0, LV_PART_MAIN);

  // Create title label
  title_label = lv_label_create(title_bar);
  lv_label_set_text(title_label, "Nebula Monitor v3.0");
  lv_obj_set_style_text_color(title_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_center(title_label);

  // Calculate automatic tile layout
  int cols = 2;
  int rows = (N_TARGETS + cols - 1) / cols;
  int available_width = 240;
  int available_height = 320 - 30; // Height minus title bar
  int tile_w = (available_width - (cols + 1) * 5) / cols; // 5px margin
  int tile_h = (available_height - (rows + 1) * 5) / rows; // 5px margin

  Serial.printf("[MAIN] Layout: %dx%d tiles, %dx%d pixels cada\n", cols, rows, tile_w, tile_h);

  // Create tiles
  for (int i = 0; i < N_TARGETS; i++) {
    int row = i / cols;
    int col = i % cols;
    int x = 5 + col * (tile_w + 5);
    int y = 35 + row * (tile_h + 5);

    // Create tile container
    tile_containers[i] = lv_obj_create(main_screen);
    lv_obj_set_size(tile_containers[i], tile_w, tile_h);
    lv_obj_set_pos(tile_containers[i], x, y);
    lv_obj_set_style_bg_color(tile_containers[i], lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_border_color(tile_containers[i], lv_color_hex(0x666666), LV_PART_MAIN);
    lv_obj_set_style_border_width(tile_containers[i], 2, LV_PART_MAIN);
    lv_obj_set_style_radius(tile_containers[i], 8, LV_PART_MAIN);

    // Create tile name label
    lv_obj_t* tile_label = lv_label_create(tile_containers[i]);
    lv_label_set_text(tile_label, targets[i].name);
    lv_obj_set_style_text_color(tile_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(tile_label, LV_FONT_DEFAULT, LV_PART_MAIN);
    lv_obj_align(tile_label, LV_ALIGN_TOP_MID, 0, 5);

    // Create status label
    lv_obj_t* status_tile_label = lv_label_create(tile_containers[i]);
    lv_label_set_text(status_tile_label, "Unknown");
    lv_obj_set_style_text_color(status_tile_label, lv_color_hex(0xCCCCCC), LV_PART_MAIN);
    lv_obj_set_style_text_font(status_tile_label, LV_FONT_DEFAULT, LV_PART_MAIN);
    lv_obj_align(status_tile_label, LV_ALIGN_BOTTOM_MID, 0, -5);

    Serial.printf("[MAIN] Tile %d criado: %s em (%d, %d)\n", i, targets[i].name, x, y);
  }

  Serial.println("[MAIN] Setup completo! Interface pronta!");
}

void loop() {
  // Handle LVGL tasks
  lv_timer_handler();

  // Handle touch input
  if (Touch::touched()) {
    int16_t raw_x, raw_y, z;
    Touch::readRaw(raw_x, raw_y, z);
    
    int screen_x, screen_y;
    Touch::mapRawToScreen(raw_x, raw_y, screen_x, screen_y);
    
    Serial.printf("[TOUCH] Touch detectado em (%d, %d)\n", screen_x, screen_y);
    
    // Check which tile was touched
    for (int i = 0; i < N_TARGETS; i++) {
      lv_area_t area;
      lv_obj_get_coords(tile_containers[i], &area);
      
      if (screen_x >= area.x1 && screen_x < area.x2 && 
          screen_y >= area.y1 && screen_y < area.y2) {
        
        // Generate random color
        uint32_t randomColor = random(0x100000, 0xFFFFFF);
        
        // Change tile color
        lv_obj_set_style_bg_color(tile_containers[i], lv_color_hex(randomColor), LV_PART_MAIN);
        lv_obj_invalidate(tile_containers[i]);
        
        // FORÇAR REFRESH IMEDIATO (CORREÇÃO DESCOBERTA!)
        lv_refr_now(lv_disp_get_default());
        
        // Update status text
        lv_obj_t* status_label = lv_obj_get_child(tile_containers[i], 1); // Second child is status
        if (status_label) {
          lv_label_set_text(status_label, "TOUCHED!");
        }
        
        Serial.printf("[TOUCH] Tile %d (%s) - Cor: #%06X - REFRESH FORÇADO!\n", 
                     i, targets[i].name, randomColor);
        break;
      }
    }
  }

  // Small delay for stability
  delay(10);
}
