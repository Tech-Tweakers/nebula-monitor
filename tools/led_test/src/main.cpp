#include <Arduino.h>

// Configuração do LED RGB (mesma do projeto principal)
const int LED_PIN_R = 16;
const int LED_PIN_G = 17;
const int LED_PIN_B = 20;
const bool LED_ACTIVE_HIGH = false; // Common anode

// Configuração PWM
const int LEDC_CHANNEL_R = 0;
const int LEDC_CHANNEL_G = 1;
const int LEDC_CHANNEL_B = 2;
const int LEDC_FREQ = 5000; // 5kHz
const int LEDC_RES_BITS = 8; // 8 bits (0-255)

// Brilho para cada cor
const int LED_BRIGHT_R = 100;
const int LED_BRIGHT_G = 100;
const int LED_BRIGHT_B = 100;

void setLED(bool r_on, bool g_on, bool b_on) {
  // Calcular brilho
  const int r_brightness = r_on ? LED_BRIGHT_R : 0;
  const int g_brightness = g_on ? LED_BRIGHT_G : 0;
  const int b_brightness = b_on ? LED_BRIGHT_B : 0;

  // Inverter para common anode se necessário
  const int r_duty = LED_ACTIVE_HIGH ? r_brightness : (255 - r_brightness);
  const int g_duty = LED_ACTIVE_HIGH ? g_brightness : (255 - g_brightness);
  const int b_duty = LED_ACTIVE_HIGH ? b_brightness : (255 - b_brightness);

  // Aplicar PWM
  ledcWrite(LEDC_CHANNEL_R, r_duty);
  ledcWrite(LEDC_CHANNEL_G, g_duty);
  ledcWrite(LEDC_CHANNEL_B, b_duty);
  
  Serial.printf("LED: R=%d G=%d B=%d (duty: R=%d G=%d B=%d)\n", 
                r_on, g_on, b_on, r_duty, g_duty, b_duty);
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("========================================");
  Serial.println("    TESTE DE LED RGB - NEBULA MONITOR");
  Serial.println("========================================");
  Serial.printf("Pinos: R=%d, G=%d, B=%d\n", LED_PIN_R, LED_PIN_G, LED_PIN_B);
  Serial.printf("Active High: %s\n", LED_ACTIVE_HIGH ? "true" : "false");
  Serial.printf("Freq: %dHz, Res: %d bits\n", LEDC_FREQ, LEDC_RES_BITS);
  Serial.println("========================================");
  
  // Configurar PWM
  ledcSetup(LEDC_CHANNEL_R, LEDC_FREQ, LEDC_RES_BITS);
  ledcSetup(LEDC_CHANNEL_G, LEDC_FREQ, LEDC_RES_BITS);
  ledcSetup(LEDC_CHANNEL_B, LEDC_FREQ, LEDC_RES_BITS);
  
  ledcAttachPin(LED_PIN_R, LEDC_CHANNEL_R);
  ledcAttachPin(LED_PIN_G, LEDC_CHANNEL_G);
  ledcAttachPin(LED_PIN_B, LEDC_CHANNEL_B);
  
  // Iniciar com LED apagado
  setLED(false, false, false);
  
  Serial.println("Iniciando teste de cores...");
  delay(1000);
}

void loop() {
  Serial.println("\n--- TESTE VERMELHO ---");
  setLED(true, false, false);
  delay(2000);
  
  Serial.println("\n--- TESTE VERDE ---");
  setLED(false, true, false);
  delay(2000);
  
  Serial.println("\n--- TESTE AZUL ---");
  setLED(false, false, true);
  delay(2000);
  
  Serial.println("\n--- TESTE AMARELO (R+G) ---");
  setLED(true, true, false);
  delay(2000);
  
  Serial.println("\n--- TESTE CIANO (G+B) ---");
  setLED(false, true, true);
  delay(2000);
  
  Serial.println("\n--- TESTE MAGENTA (R+B) ---");
  setLED(true, false, true);
  delay(2000);
  
  Serial.println("\n--- TESTE BRANCO (R+G+B) ---");
  setLED(true, true, true);
  delay(2000);
  
  Serial.println("\n--- TESTE APAGADO ---");
  setLED(false, false, false);
  delay(2000);
  
  Serial.println("\n--- TESTE PISCANDO AZUL ---");
  for (int i = 0; i < 5; i++) {
    setLED(false, false, true);
    delay(500);
    setLED(false, false, false);
    delay(500);
  }
  
  Serial.println("\n--- CICLO COMPLETO ---");
  delay(2000);
}
