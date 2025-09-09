# SD Config Uploader

Sketch simples para escrever `config.env` diretamente no SD Card.

## Como usar:

1. **Editar configuração:**
   - Edite o `configContent` no arquivo `src/main.cpp`
   - Modifique as configurações conforme necessário

2. **Compilar e upload:**
   ```bash
   cd sd_config_uploader
   pio run --target upload    # Upload do sketch
   ```

3. **Executar:**
   - Abrir monitor serial (115200 baud)
   - O sketch vai escrever o config.env diretamente no SD

4. **Resultado:**
   - SD Card terá o arquivo `/config.env` com as configurações
   - Inserir no ESP32 principal
   - O sistema vai detectar e sincronizar automaticamente

## Requisitos:

- SD Card inserido no ESP32
- Apenas a biblioteca SD (sem SPIFFS)

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
