# SD Config Uploader

Sketch para copiar `config.env` do SPIFFS para o SD Card sem modificações.

## Como usar:

1. **Preparar config.env:**
   - Edite o arquivo `config.env` na pasta do uploader
   - Ou copie um config.env existente para esta pasta

2. **Compilar e upload:**
   ```bash
   cd sd_config_uploader
   pio run -e esp32dev --target uploadfs  # Upload do config.env para SPIFFS
   pio run -e esp32dev --target upload    # Upload do sketch
   ```

3. **Executar:**
   - Abrir monitor serial (115200 baud)
   - O sketch vai copiar automaticamente o config.env do SPIFFS para o SD

4. **Resultado:**
   - SD Card terá o arquivo `/config.env` (cópia exata do config.env da pasta)
   - Agora você pode editar no PC
   - Inserir no ESP32 principal
   - O sistema vai detectar e sincronizar automaticamente

## Requisitos:

- SD Card inserido no ESP32
- config.env na pasta `data/` do uploader
- Execute `pio run --target uploadfs` para fazer upload do config.env para o SPIFFS

## Pinout:

- SD CS: Pin 5 (mesmo do projeto principal)

## Funcionalidades:

- ✅ Copia arquivo byte por byte (sem modificações)
- ✅ Verifica integridade da cópia
- ✅ Logs detalhados do processo
- ✅ Tratamento de erros robusto

## Depois de usar:

- Deletar esta pasta
- Usar o projeto principal normalmente
