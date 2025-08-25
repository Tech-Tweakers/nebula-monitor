#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

TFT_eSPI tft = TFT_eSPI();

// --- Touch XPT2046 (SPI dedicado)
#define XPT2046_IRQ  36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39  // (input-only, ok)
#define XPT2046_CLK  25
#define XPT2046_CS   33

SPIClass touchscreenSPI(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

// --- Tela
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

// --- Botão (topo, centralizado)
#define BTN_WIDTH   120
#define BTN_HEIGHT   40
#define BTN_Y        10
int BTN_X = (SCREEN_WIDTH - BTN_WIDTH) / 2;

// --- Estado
bool isPressed = false;

// --- Backlight
static const int BL_PIN = 27;
static inline void setBL(bool on){ pinMode(BL_PIN, OUTPUT); digitalWrite(BL_PIN, on ? HIGH : LOW); }

// --- Desenho
void drawButton(bool pressed) {
  uint16_t color = pressed ? TFT_DARKGREY : TFT_LIGHTGREY;
  tft.fillRoundRect(BTN_X, BTN_Y, BTN_WIDTH, BTN_HEIGHT, 8, color);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_BLACK, color); // texto com fundo do próprio botão
  tft.drawCentreString("Clique", BTN_X + BTN_WIDTH/2, BTN_Y + BTN_HEIGHT/2 - 6, 2);
}

// --- Hit test
bool isTouchInsideButton(int x, int y) {
  return x >= BTN_X && x <= (BTN_X + BTN_WIDTH) &&
         y >= BTN_Y && y <= (BTN_Y + BTN_HEIGHT);
}

void setup() {
  Serial.begin(115200);
  delay(100);

  setBL(true);

  // Touch SPI (pinos custom VSPI)
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(2); // igual ao TFT

  // TFT
  tft.init();
  tft.setRotation(2); // "Rotation 2" que você curtiu
  tft.fillScreen(TFT_BLACK);

  drawButton(false);
}

void loop() {
  if (touchscreen.tirqTouched() && touchscreen.touched()) {
    TS_Point p = touchscreen.getPoint();

    // Calibração bruta -> pixels (ajuste fino nesses limites se quiser)
    int x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    int y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);

    // Como estamos em rot 2, espelha os dois eixos:
    x = SCREEN_WIDTH  - x;
    y = SCREEN_HEIGHT - y;

    bool inside = isTouchInsideButton(x, y);
    if (inside && !isPressed) {
      isPressed = true;
      drawButton(true);
      Serial.println("Botão pressionado!");
    } else if (!inside && isPressed) {
      isPressed = false;
      drawButton(false);
    }

    delay(80); // debounce leve
  }
}
