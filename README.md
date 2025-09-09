# 🌌 Nebula Monitor v2.4

> **ESP32 TFT Network Monitor Dashboard** - Clean architecture network monitoring with manual garbage collection, SSL thread safety, and 24/7 stability

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

**Nebula Monitor v2.4** is a production-ready network monitoring dashboard for ESP32 TFT displays. Built with clean architecture principles, it provides 24/7 stability through manual garbage collection, SSL thread safety, and intelligent memory management.

### 🎪 Key Features

- **🏗️ Clean Architecture**: Modular design with dependency injection
- **🧠 Manual Garbage Collection**: Prevents memory leaks and system reboots
- **🔒 SSL Thread Safety**: Mutex-managed SSL operations for stability
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

### 🔒 SSL Thread Safety
- **Mutex Management**: Thread-safe SSL operations
- **RAII Wrapper**: Automatic lock/unlock with SSLLock class
- **Statistics Tracking**: Performance monitoring and debugging
- **Timeout Management**: Configurable operation timeouts

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


## 🔧 Hardware Requirements

- **ESP32 Development Board** (ESP32-DevKitC or similar)
- **TFT Display**: ST7789 Driver (240x320 resolution)
- **Touch Controller**: XPT2046 Resistive Touch
- **Recommended**: Cheap Yellow Board (CYB) - pre-configured ESP32 + TFT module

### Pin Configuration
| Component | ESP32 Pin | Function |
|-----------|-----------|----------|
| TFT_MOSI | GPIO 13 | SPI Data |
| TFT_SCLK | GPIO 14 | SPI Clock |
| TFT_CS | GPIO 15 | Chip Select |
| TFT_DC | GPIO 2 | Data/Command |
| TFT_BL | GPIO 27 | Backlight |
| TOUCH_CS | GPIO 33 | Touch Select |
| TOUCH_IRQ | GPIO 36 | Touch Interrupt |

## 📦 Dependencies

```ini
lib_deps =
  TFT_eSPI                    # TFT display driver
  XPT2046_Touchscreen         # Touch controller
  lvgl/lvgl@^8.4.0           # UI framework
  ArduinoJson@^6.21.5         # JSON parsing
  HTTPClient@^2.0.0           # HTTP client
  WiFiClientSecure@^2.0.0     # HTTPS support
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
WIFI_SSID=YourWiFiName
WIFI_PASS=YourWiFiPassword
TELEGRAM_BOT_TOKEN=your_bot_token_here
TELEGRAM_CHAT_ID=your_chat_id_here
MAX_FAILURES_BEFORE_ALERT=3
ALERT_COOLDOWN_MS=300000
```

## 📱 User Interface

### Main Screen Layout
```
┌─────────────────────────────────┐
│        Nebula Monitor v2.4      │ ← Title Bar
├─────────────────────────────────┤
│ ┌─────────────────────────────┐ │
│ │ Proxmox HV      [123 ms]    │ │ ← Status Items
│ │ Router #1       [45 ms]     │ │
│ │ Router #2       [DOWN]      │ │
│ │ Polaris API     [OK]        │ │
│ │ Polaris INT     [OK]        │ │
│ │ Polaris WEB     [OK]        │ │
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

## 🔍 Network Monitoring

### Hybrid Scanning
- **PING**: Basic HTTP GET requests (5s timeout)
- **Health Check**: API endpoint verification with JSON parsing
- **Sequential**: One target at a time for stability
- **30s intervals**: Optimized for 24/7 operation

### Supported Protocols
- **HTTP/HTTPS**: Full protocol support
- **JSON Parsing**: Health check response analysis
- **Auto Fallback**: HTTP fallback for HTTPS failures

## 🛠️ Development

### Build Commands
```bash
pio run --target clean    # Clean build
pio run                   # Build project
pio run --target upload   # Upload firmware
pio device monitor        # Monitor serial output
```

### Debug Logging
```env
# In data/config.env
DEBUG_LOGS_ENABLED=true
TOUCH_LOGS_ENABLED=true
ALL_LOGS_ENABLED=true
```

### Common Issues
1. **WiFi Connection Failed**: Check SSID/password in config.env
2. **Display Not Working**: Verify pin connections
3. **Touch Not Responding**: Check touch calibration
4. **Telegram Alerts Not Working**: Check bot token and chat ID

## 📁 Project Structure

```
nebula-monitor/
├── src/
│   ├── core/
│   │   ├── application/        # NetworkMonitor
│   │   ├── domain/            # Target, Alert, Status
│   │   └── infrastructure/    # MemoryManager, SSLMutexManager, HttpClient, etc.
│   ├── tasks/                 # TaskManager
│   ├── ui/                    # DisplayManager, TouchHandler, LEDController
│   └── config/                # ConfigLoader
├── data/
│   └── config.env             # External configuration
├── include/
│   ├── lv_conf.h              # LVGL configuration
│   └── User_Setup.h           # TFT configuration
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

### Debug Output
```bash
pio device monitor --baud 115200
```

Key debug messages: `[MAIN]`, `[NET]`, `[SCANNER]`, `[TELEGRAM]`, `[TOUCH]`, `[FOOTER]`

## 📈 Performance

### System Performance
- **Flash**: 93.5% (1,225,821 bytes of 1,310,720 bytes)
- **RAM**: 33.2% (108,916 bytes of 327,680 bytes)
- **Free Heap**: ~220KB available for operations

### Network Performance
- **Scan Interval**: 30 seconds per cycle
- **HTTP Timeout**: 5 seconds (standard), 7+ seconds (cloud services)
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
- **SSL Safety**: Thread-safe operations with mutex
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
- Documentation improvements

## 📄 License

MIT License - see [LICENSE](LICENSE) file for details.

---

<div align="center">

Made with ❤️ for the ESP32 community

</div>