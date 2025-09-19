# 🌌 Nebula Monitor v2.5

> **ESP32 TFT Network Monitor Dashboard** - Production-ready network monitoring with SSL protection, manual garbage collection, and 24/7 stability

[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32-blue.svg)](https://platformio.org/)
[![LVGL](https://img.shields.io/badge/LVGL-8.4.0-green.svg)](https://lvgl.io/)
[![TFT_eSPI](https://img.shields.io/badge/TFT_eSPI-Latest-orange.svg)](https://github.com/Bodmer/TFT_eSPI)
[![Telegram](https://img.shields.io/badge/Telegram-Alerts-blue.svg)](https://telegram.org/)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![GitHub](https://img.shields.io/badge/GitHub-Tech--Tweakers-black.svg)](https://github.com/Tech-Tweakers)

## 📋 Table of Contents

- [🎯 Overview](#-overview)
- [✨ Features](#-features)
- [🔧 Hardware Requirements](#-hardware-requirements)
- [🚀 Quick Start](#-quick-start)
- [⚙️ Configuration](#️-configuration)
- [📱 User Interface](#-user-interface)
- [🛠️ Development](#️-development)
- [📁 Project Structure](#-project-structure)
- [🐛 Troubleshooting](#-troubleshooting)
- [📈 Performance](#-performance)
- [🤝 Contributing](#-contributing)

## 🎯 Overview

**Nebula Monitor v2.5** is a production-ready network monitoring dashboard for ESP32 TFT displays. Built with clean architecture principles, it provides 24/7 stability through SSL protection, manual garbage collection, and intelligent memory management.

### 🎪 Key Features

- **🏗️ Clean Architecture**: Modular design with dependency injection
- **🔒 SSL Protection**: Safe handling of HTTPS endpoints without crashes
- **🧠 Manual Garbage Collection**: Prevents memory leaks and system reboots
- **📊 Dynamic Footer**: 3 optimized modes with real-time data
- **🚨 Telegram Alerts**: Smart notifications with cooldown management
- **🔄 Hybrid Monitoring**: PING + Health Check for comprehensive coverage
- **⚡ 24/7 Stability**: Tested for continuous operation without reboots
- **📱 Touch Interface**: Responsive LVGL-based UI with visual feedback

## ✨ Features

### 🧠 Memory Management System
- **Manual Garbage Collection**: Prevents memory leaks and system reboots
- **String Pool**: Optimized string allocation and deallocation
- **Memory Monitoring**: Real-time heap and stack usage tracking
- **Watchdog Feeding**: Automatic ESP32 watchdog timer management
- **Emergency Cleanup**: Critical memory pressure handling

### 🔒 SSL Security
- **Thread Safety**: Mutex-managed SSL operations
- **Context Cleanup**: Proper SSL resource management
- **Timeout Management**: Configurable operation timeouts
- **Error Recovery**: Graceful handling of SSL failures

### 📊 Dynamic Footer (3 Modes)
- **System Overview**: `Alerts: X | On: Y/Z | Up: HH:MM`
- **Network Info**: `IP: 192.168.1.162 | -45 dBm`
- **Performance**: `Cpu: 45% | Ram: 32% | Heap: 107KB`

### 🚨 Telegram Alerts
- **Smart Thresholds**: Configurable failure count (default: 3)
- **Cooldown Management**: 5-minute alert intervals
- **Recovery Notifications**: Service restoration alerts
- **Rich Formatting**: Emojis and detailed information

### 🔄 Hybrid Monitoring
- **PING**: Basic connectivity checks
- **Health Check**: API endpoint verification with JSON parsing
- **Multi-target**: Up to 6 simultaneous targets
- **Real-time Latency**: Response time tracking
- **Protocol Support**: HTTP and HTTPS with proper SSL handling

## 🔧 Hardware Requirements

- **Board**: ESP32-2432S028R (new CYB/CYD variant)
- **TFT Display**: ST7789 (240x320), rotation 2
- **Touch Controller**: XPT2046 (resistive)
- **Storage**: microSD (VSPI) + SPIFFS (runtime)

### Pin Configuration

SPI buses:
- HSPI (TFT + Touch, shared bus): MOSI=13, MISO=12, SCLK=14
- VSPI (SD card): MOSI=23, MISO=19, SCLK=18

| Peripheral | Signal     | ESP32 Pin | Notes                          |
|------------|------------|-----------|--------------------------------|
| TFT (HSPI) | MOSI       | GPIO 13   |                                |
|            | MISO       | GPIO 12   |                                |
|            | SCLK       | GPIO 14   |                                |
|            | CS         | GPIO 15   | `TFT_CS`                       |
|            | DC         | GPIO 2    | `TFT_DC`                       |
|            | RST        | NC (-1)   | Not connected                  |
|            | Backlight  | GPIO 27   | `TFT_BL` (active HIGH)         |
| Touch      | MOSI       | GPIO 13   | Shared HSPI                    |
| (XPT2046)  | MISO       | GPIO 12   | Shared HSPI                    |
|            | SCLK       | GPIO 14   | Shared HSPI                    |
|            | CS         | GPIO 33   | `TOUCH_CS`                     |
|            | IRQ        | GPIO 36   | `TOUCH_IRQ`                    |
| SD Card    | MOSI       | GPIO 23   | VSPI                           |
|            | MISO       | GPIO 19   | VSPI                           |
|            | SCLK       | GPIO 18   | VSPI                           |
|            | CS         | GPIO 5    | `SD_CS`                        |
| RGB LED    | R          | GPIO 16   | Default; configurable in env   |
|            | G          | GPIO 17   | Default; configurable in env   |
|            | B          | GPIO 20   | Default; configurable in env   |

Notes:
- TFT and Touch share HSPI; each has its own CS.
- SD uses VSPI with `CS=GPIO 5`.
- Backlight on `GPIO 27` (active HIGH). TFT reset not connected (`TFT_RST=-1`).
- LED defaults: R=16, G=17, B=20, active LOW (common anode). Overridable via `data/config.env`.

## 📦 Dependencies

```ini
lib_deps =
  TFT_eSPI                    # TFT display driver
  XPT2046_Touchscreen         # Touch controller
  lvgl/lvgl@^8.4.0           # UI framework
  ArduinoJson@^6.21.5         # JSON parsing
  HTTPClient@^2.0.0           # HTTP client
  WiFiClientSecure@^2.0.0     # HTTPS support
  NTPClient@^3.2.1            # NTP time sync
  SD@^2.0.0                   # SD card support
  SPIFFS@^2.0.0               # SPIFFS filesystem
```

## 🚀 Quick Start

1. **Clone and setup**:
```bash
git clone https://github.com/Tech-Tweakers/nebula-monitor.git
cd nebula-monitor
pip install platformio
```

2. **Configure** `data/config.env`:
```env
WIFI_SSID=YourWiFiName
WIFI_PASS=YourWiFiPassword
TELEGRAM_BOT_TOKEN=your_bot_token_here
TELEGRAM_CHAT_ID=your_chat_id_here
TARGET_1=Proxmox HV|http://192.168.1.128:8006/||PING
TARGET_2=Router|http://192.168.1.1||PING
TARGET_3=API Server|https://api.example.com|/health|HEALTH_CHECK
```

3. **Upload and run**:
```bash
pio run --target uploadfs  # Upload config
pio run --target upload    # Upload firmware
pio device monitor         # Monitor output
```

## ⚙️ Configuration

All settings managed via `data/config.env` file on SPIFFS filesystem.

### Target Format
```
TARGET_N=NAME|URL|HEALTH_ENDPOINT|MONITOR_TYPE
```
- **NAME**: Display name
- **URL**: Full URL (http:// or https://)
- **HEALTH_ENDPOINT**: Health check path (empty for PING)
- **MONITOR_TYPE**: `PING` or `HEALTH_CHECK`

### Key Settings
```env
# WiFi Configuration
WIFI_SSID=YourWiFiName
WIFI_PASS=YourWiFiPassword

# Telegram Bot
TELEGRAM_BOT_TOKEN=your_bot_token_here
TELEGRAM_CHAT_ID=your_chat_id_here
TELEGRAM_ENABLED=true

# Alert Configuration
MAX_FAILURES_BEFORE_ALERT=3
ALERT_COOLDOWN_MS=300000
ALERT_RECOVERY_COOLDOWN_MS=60000

# Performance
SCAN_INTERVAL_MS=90000
HTTP_TIMEOUT_MS=2000

# Debug (optional)
DEBUG_LOGS_ENABLED=false
SILENT_MODE=true
```

### Health Check Patterns
```env
# Healthy response patterns (case-insensitive)
HEALTH_CHECK_HEALTHY_PATTERNS="status":"healthy","status":"ok","status":"up","status":"running","health":"ok","health":"healthy","health":"up","ok","healthy","up"

# Unhealthy response patterns (case-insensitive)
HEALTH_CHECK_UNHEALTHY_PATTERNS="status":"unhealthy","status":"down","status":"error","status":"failed","health":"unhealthy","health":"down","502 bad gateway","503 service unavailable","504 gateway timeout","500 internal server error"
```

## 📱 User Interface

### Main Screen Layout
```
┌─────────────────────────────────┐
│        Nebula Monitor v2.5      │ ← Title Bar
├─────────────────────────────────┤
│ ┌─────────────────────────────┐ │
│ │ Proxmox HV      [123 ms]    │ │ ← Status Items
│ │ Router #1       [45 ms]     │ │
│ │ Router #2       [DOWN]      │ │
│ │ API Server      [OK]        │ │
│ │ Web Service     [OK]        │ │
│ │ Database        [OK]        │ │
│ └─────────────────────────────┘ │
├─────────────────────────────────┤
│ Alerts: 0 | On: 6/6 | Up: 01:23 │ ← Dynamic Footer (3 modes)
└─────────────────────────────────┘
```

### Footer Modes (Touch to cycle)
- **System Overview**: `Alerts: X | On: Y/Z | Up: HH:MM`
- **Network Info**: `IP: 192.168.1.162 | -45 dBm`
- **Performance**: `Cpu: 45% | Ram: 32% | Heap: 107KB`

### Status Colors
- 🟢 **Green**: Target UP with good latency (<500ms)
- 🔵 **Blue**: Target UP with slow latency (≥500ms)
- 🔴 **Red**: Target DOWN
- ⚪ **Gray**: Target UNKNOWN (timeout/error)

## 🔍 Network Monitoring

### Hybrid Scanning
- **PING**: Basic HTTP GET requests (5s timeout)
- **Health Check**: API endpoint verification with JSON parsing
- **Sequential**: One target at a time for stability
- **90s intervals**: Optimized for 24/7 operation
- **SSL Support**: Safe handling of HTTPS endpoints

### Supported Protocols
- **HTTP/HTTPS**: Full protocol support with SSL protection
- **JSON Parsing**: Health check response analysis
- **Auto Fallback**: HTTP fallback for HTTPS failures
- **Timeout Management**: Intelligent timeout handling per service type

## 🛠️ Development

### Build Commands
```bash
pio run --target clean    # Clean build
pio run                   # Build project
pio run --target upload   # Upload firmware
pio run --target uploadfs # Upload SPIFFS filesystem
pio device monitor        # Monitor serial output
```

### Debug Logging
```env
# In data/config.env
DEBUG_LOGS_ENABLED=true
TOUCH_LOGS_ENABLED=true
TELEGRAM_LOGS_ENABLED=true
ALL_LOGS_ENABLED=true
SILENT_MODE=false
```

### Common Issues
1. **WiFi Connection Failed**: Check SSID/password in config.env
2. **Display Not Working**: Verify pin connections
3. **Touch Not Responding**: Check touch calibration
4. **Telegram Alerts Not Working**: Check bot token and chat ID
5. **Random Reboots**: Ensure using v2.5+ with SSL protection

## 📁 Project Structure

```
nebula-monitor/
├── src/
│   ├── core/
│   │   ├── domain/            # Target, Alert, Status, NetworkMonitor
│   │   └── infrastructure/    # MemoryManager, SSLMutexManager, HttpClient, etc.
│   ├── ui/                    # DisplayManager, TouchHandler, LEDController
│   ├── config/                # ConfigLoader
│   └── main.cpp               # Application entry point
├── data/
│   └── config.env             # External configuration
├── include/
│   ├── lv_conf.h              # LVGL configuration
│   └── User_Setup.h           # TFT configuration
├── tools/                     # Utility tools
├── platformio.ini             # Build configuration
└── README.md                  # This file
```

## 🐛 Troubleshooting

### Common Issues

**Configuration not loading**:
- Ensure `config.env` exists in `data/` directory
- Upload SPIFFS: `pio run --target uploadfs`
- Check file format (no spaces around `=`)

**WiFi connection fails**:
- Check SSID/password in `config.env`
- Verify 2.4GHz network (ESP32 doesn't support 5GHz)
- Check signal strength

**Display issues**:
- Verify pin connections
- Check TFT driver settings
- Ensure `TFT_INVERSION_ON=1` is set

**Touch not responding**:
- Check touch calibration values
- Verify TOUCH_CS pin (GPIO 33)
- Recalibrate touch coordinates

**Telegram alerts not working**:
- Verify bot token and chat ID in `config.env`
- Send a message to the bot first
- Check network connectivity

**Random reboots**:
- Update to v2.5+ for SSL protection
- Check for problematic HTTPS endpoints
- Monitor system logs for errors

### Debug Output
```bash
pio device monitor --baud 115200
```

Key debug messages: `[MAIN]`, `[NETWORK_MONITOR]`, `[HTTP]`, `[TELEGRAM]`, `[TOUCH]`, `[MEMORY_MANAGER]`

## 📈 Performance

### System Performance
- **Flash**: 97.7% (1,280,497 bytes of 1,310,720 bytes)
- **RAM**: 33.4% (109,440 bytes of 327,680 bytes)
- **Free Heap**: ~218KB available for operations
- **Stack Usage**: Optimized to prevent overflow

### Network Performance
- **Scan Interval**: 90 seconds per cycle
- **HTTP Timeout**: 2-8 seconds (configurable per service type)
- **Sequential Scanning**: One target at a time for stability
- **Alert Cooldown**: 5 minutes between alerts

### Memory Management
- **Garbage Collection**: Automatic every 2 minutes
- **String Pool**: 10-string pool for optimization
- **Watchdog Feeding**: Continuous to prevent reboots
- **Memory Monitoring**: Real-time heap and stack tracking

### 24/7 Stability
- **Tested**: Continuous operation without reboots
- **Memory Leaks**: Prevented by manual garbage collection
- **SSL Safety**: Thread-safe operations with proper cleanup
- **Error Handling**: Robust error recovery mechanisms

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/amazing-feature`
3. Commit changes: `git commit -m 'Add amazing feature'`
4. Push to branch: `git push origin feature/amazing-feature`
5. Open a Pull Request

### Contribution Areas
- Hardware support for new displays
- Network monitoring features
- UI/UX improvements
- Performance optimizations
- SSL security enhancements
- Documentation improvements

## 📄 License

MIT License - see [LICENSE](LICENSE) file for details.

---

<div align="center">

Made with ❤️ for the ESP32 community

**Nebula Monitor v2.5** - Production-ready network monitoring! 🌌✨

</div>