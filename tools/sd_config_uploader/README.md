# SD Config Uploader

Sketch temporário para copiar `config.env` do SPIFFS para o SD Card.

## Como usar:

1. **Compilar e upload:**
   ```bash
   cd sd_config_uploader
   pio run -e esp32dev
   pio run -e esp32dev --target upload
   ```

2. **Executar:**
   - Abrir monitor serial (115200 baud)
   - O sketch vai copiar automaticamente o config.env do SPIFFS para o SD

3. **Resultado:**
   - SD Card terá o arquivo `/config.env`
   - Agora você pode editar no PC
   - Inserir no ESP32 principal
   - O sistema vai detectar mudanças e reiniciar

## Requisitos:

- SD Card inserido no ESP32
- config.env já existe no SPIFFS (execute `pio run --target uploadfs` no projeto principal primeiro)

## Pinout:

- SD CS: Pin 5 (mesmo do projeto principal)

## Depois de usar:

- Deletar esta pasta
- Usar o projeto principal normalmente
