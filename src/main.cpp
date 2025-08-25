#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

TFT_eSPI tft = TFT_eSPI();

// --- Touch XPT2046 (SPI dedicado)
#define XPT2046_IRQ  36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
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

// --- Backlight
static const int BL_PIN = 27;
static inline void setBL(bool on){ pinMode(BL_PIN, OUTPUT); digitalWrite(BL_PIN, on ? HIGH : LOW); }

// --- Utils de cor (RGB888 -> RGB565)
static inline uint16_t RGB(uint8_t r,uint8_t g,uint8_t b){
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
static inline bool isDark565(uint16_t c){
  uint8_t r = ((c >> 11) & 0x1F) * 255 / 31;
  uint8_t g = ((c >> 5)  & 0x3F) * 255 / 63;
  uint8_t b = ( c        & 0x1F) * 255 / 31;
  float L = 0.2126f*r + 0.7152f*g + 0.0722f*b;
  return L < 140.0f;
}

// --- Paleta que gira a cada clique
const uint16_t PALETTE[] = {
  RGB(220, 60, 60),   // vermelho
  RGB(255,160,  0),   // laranja
  RGB(255,220,  0),   // amarelo
  RGB(  0,200, 90),   // verde
  RGB(  0,180,200),   // ciano
  RGB( 70,100,255),   // azul
  RGB(180, 80,220),   // roxo
  RGB(230,110,180)    // rosa
};
const int PALETTE_SIZE = sizeof(PALETTE)/sizeof(PALETTE[0]);
int colorIndex = 0;
uint16_t baseColor = PALETTE[0];

// --- HUD de métricas
void showDrawTime(uint32_t us){
  tft.fillRect(0, SCREEN_HEIGHT - 18, SCREEN_WIDTH, 18, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  char buf[32];
  snprintf(buf, sizeof(buf), "%lu us", (unsigned long)us);
  tft.drawCentreString(buf, SCREEN_WIDTH/2, SCREEN_HEIGHT - 9, 2);
}

// --- Desenho do botão (sem "escurecer")
uint32_t drawButton() {
  uint32_t t0 = micros();
  tft.fillRoundRect(BTN_X, BTN_Y, BTN_WIDTH, BTN_HEIGHT, 8, baseColor);
  tft.setTextDatum(MC_DATUM);
  uint16_t txt = isDark565(baseColor) ? TFT_WHITE : TFT_BLACK;
  tft.setTextColor(txt, baseColor);  // fundo do texto = cor do botão
  tft.drawCentreString("Clique", BTN_X + BTN_WIDTH/2, BTN_Y + BTN_HEIGHT/2 - 6, 2);
  return micros() - t0;
}

// --- Hit test
bool isTouchInsideButton(int x, int y) {
  return x >= BTN_X && x <= (BTN_X + BTN_WIDTH) &&
         y >= BTN_Y && y <= (BTN_Y + BTN_HEIGHT);
}

// --- Estado de toque (edge detection)
bool touching = false;
uint32_t lastTapMs = 0;
const uint16_t DEBOUNCE_MS = 120;

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
  tft.setRotation(2);         // "Rotation 2"
  tft.fillScreen(TFT_BLACK);

  showDrawTime(drawButton());
}

void loop() {
  bool nowTouching = touchscreen.touched();

  // Borda de subida (finger down) = 1 clique
  if (nowTouching && !touching) {
    TS_Point p = touchscreen.getPoint();

    // Calibração bruta -> pixels (ajuste fino se necessário)
    int x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    int y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);

    // Rotação 2: espelha os dois eixos
    x = SCREEN_WIDTH  - x;
    y = SCREEN_HEIGHT - y;

    uint32_t nowMs = millis();
    if (nowMs - lastTapMs > DEBOUNCE_MS && isTouchInsideButton(x, y)) {
      colorIndex = (colorIndex + 1) % PALETTE_SIZE;
      baseColor  = PALETTE[colorIndex];
      uint32_t us = drawButton();
      showDrawTime(us);
      Serial.print("Tap draw time: "); Serial.print(us); Serial.println(" us");
      lastTapMs = nowMs;
    }
  }

  touching = nowTouching;
}
