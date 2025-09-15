# Debug Flags Testing Tool

Este é um utilitário para testar o sistema de flags de debug implementado no projeto ESP32 TFT Network Monitor.

## Funcionalidades

O sistema de logging condicional permite controlar a saída de logs através de flags de configuração:

### Flags Disponíveis

- `DEBUG_LOGS_ENABLED`: Controla logs de debug gerais
- `TOUCH_LOGS_ENABLED`: Controla logs específicos de touch
- `ALL_LOGS_ENABLED`: Controla todos os logs (exceto ERROR e CRITICAL)

### Tipos de Log

1. **LOG_DEBUG**: Logs de debug (controlados por DEBUG_LOGS_ENABLED ou ALL_LOGS_ENABLED)
2. **LOG_TOUCH**: Logs de touch (controlados por TOUCH_LOGS_ENABLED ou ALL_LOGS_ENABLED)
3. **LOG_INFO**: Logs informativos (controlados por ALL_LOGS_ENABLED)
4. **LOG_WARN**: Logs de aviso (controlados por ALL_LOGS_ENABLED)
5. **LOG_ERROR**: Logs de erro (sempre habilitados)
6. **LOG_CRITICAL**: Logs críticos (sempre habilitados)

### Logs por Serviço

- **LOG_TELEGRAM**: Logs do serviço Telegram
- **LOG_DISPLAY**: Logs do gerenciador de display
- **LOG_HTTP**: Logs do cliente HTTP
- **LOG_MEMORY**: Logs do gerenciador de memória
- **LOG_NETWORK**: Logs do monitor de rede
- **LOG_CONFIG**: Logs do carregador de configuração
- **LOG_MAIN**: Logs da função principal

## Como Usar

1. Compile o projeto:
   ```bash
   pio run
   ```

2. Faça upload para o ESP32:
   ```bash
   pio run --target upload
   ```

3. Monitore a saída serial:
   ```bash
   pio device monitor
   ```

## Configuração

As flags podem ser configuradas no arquivo `data/config.env`:

```env
# Debug Configuration
DEBUG_LOGS_ENABLED=true
TOUCH_LOGS_ENABLED=true
ALL_LOGS_ENABLED=true
```

## Teste

O programa de teste demonstra:

1. **Todos os logs habilitados**: Mostra todos os tipos de log
2. **Todos os logs desabilitados**: Mostra apenas logs de erro
3. **Flags individuais**: Testa cada flag separadamente

## Resultado Esperado

- Com `ALL_LOGS_ENABLED=true`: Todos os logs aparecem
- Com `ALL_LOGS_ENABLED=false`: Apenas logs de erro aparecem
- Com flags individuais: Apenas os logs correspondentes aparecem

## Implementação

O sistema usa macros condicionais que verificam as flags de configuração antes de executar os comandos de logging. Isso garante que:

1. Logs desabilitados não consomem recursos de CPU
2. Strings de log não são processadas quando desabilitadas
3. O sistema é eficiente em termos de memória
