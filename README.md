# 🌌 Nebula Monitor v2.5

> **ESP32 TFT Network Monitor Dashboard** - Complete modularization with Clean Architecture, intelligent timeout management, and 24/7 stability

[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32-blue.svg)](https://platformio.org/)
[![LVGL](https://img.shields.io/badge/LVGL-8.3.11-green.svg)](https://lvgl.io/)
[![TFT_eSPI](https://img.shields.io/badge/TFT_eSPI-Latest-orange.svg)](https://github.com/Bodmer/TFT_eSPI)
[![Telegram](https://img.shields.io/badge/Telegram-Alerts-blue.svg)](https://telegram.org/)
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/Tech-Tweakers/nebula-monitor)
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

**Nebula Monitor v2.5** is a production-ready network monitoring dashboard for ESP32 TFT displays. Built with complete Clean Architecture modularization, it provides 24/7 stability through intelligent timeout management, automatic alert reset, and full system control.

### 🎪 Key Features

- **🏗️ Complete Clean Architecture**: Full modularization with dedicated folders
- **⏱️ Intelligent Timeout Management**: 10s timeout with UNKNOWN status for slow connections
- **🔄 Automatic Alert Reset**: Clean state after recovery for consistent behavior
- **📅 Real-time Date/Time**: NTP-synchronized timestamps in all messages
- **🚫 Watchdog Bypass**: Full application control without system interference
- **💓 Aggressive Heartbeat**: 500ms watchdog feeding for maximum stability
- **🚨 Enhanced Telegram Alerts**: Rich analytics with date/time information
- **🔄 Hybrid Monitoring**: PING + Health Check with UNKNOWN status support
- **⚡ 24/7 Stability**: Tested for continuous operation without reboots
- **📱 Touch Interface**: Responsive LVGL-based UI with visual feedback

## ✨ Features

### 🧠 Advanced Memory Management
- **Intelligent Garbage Collection**: Deferred during active scans to prevent interruptions
- **String Pool**: Optimized string allocation and deallocation
- **Memory Monitoring**: Real-time heap and stack usage tracking
- **Watchdog Bypass**: Full application control without system interference
- **Emergency Cleanup**: Critical memory pressure handling
- **Heartbeat System**: Aggressive 500ms feeding for maximum stability

### ⏱️ Intelligent Timeout Management
- **10s Timeout**: Prevents system hangs on slow connections
- **UNKNOWN Status**: New status for timeout scenarios (first implementation)
- **Smart Recovery**: Automatic retry in next scan cycle
- **Connection Quality Metrics**: UNKNOWN status provides network quality insights
- **Race Condition Prevention**: GC deferred during active scans

### 🔒 SSL Thread Safety
- **Mutex Management**: Thread-safe SSL operations
- **RAII Wrapper**: Automatic lock/unlock with SSLLock class
- **Statistics Tracking**: Performance monitoring and debugging
- **Timeout Management**: Configurable operation timeouts

### 📊 Dynamic Footer (3 Modes)
- **System Overview**: `Alerts: X | On: Y/Z | Up: HH:MM`
- **Network Info**: `IP: 192.168.1.162 | -45 dBm`
- **Performance**: `Cpu: 45% | Ram: 32% | Heap: 107KB`

### 🚨 Enhanced Telegram Alerts
- **Smart Thresholds**: Configurable failure count (default: 3)
- **Cooldown Management**: 5-minute alert intervals
- **Recovery Notifications**: Service restoration alerts with analytics
- **Real-time Timestamps**: NTP-synchronized date/time in all messages
- **Automatic Alert Reset**: Clean state after recovery for consistent behavior
- **Rich Analytics**: First failure time, alert start time, recovery time
- **Rich Formatting**: Emojis and detailed information

### 🔄 Enhanced Hybrid Monitoring
- **PING**: Basic connectivity checks with 10s timeout
- **Health Check**: API endpoint verification with JSON parsing
- **Multi-target**: Up to 6 simultaneous targets
- **Real-time Latency**: Response time tracking
- **UNKNOWN Status**: New status for timeout scenarios (first implementation)
- **Smart Recovery**: Automatic retry in next scan cycle
- **Connection Quality Metrics**: UNKNOWN provides network quality insights


## 🔧 Hardware Requirements

- **Board**: ESP32-2432S028R (new CYB/CYD variant)
- **TFT Display**: ST7789 (240x320), rotation 2
- **Touch Controller**: XPT2046 (resistive)
- **Storage**: microSD (VSPI)
- **Note**: This README is the canonical documentation for this newer CYB variant; use it as the reference.

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
- Configuration is provided via build flags in `platformio.ini` (`USER_SETUP_LOADED=1`); `include/User_Setup.h` is ignored.

## 📦 Dependencies

```ini
lib_deps =
  TFT_eSPI                    # TFT display driver
  XPT2046_Touchscreen         # Touch controller
  lvgl/lvgl@^8.3.11           # UI framework
  ArduinoJson@^6.21.3         # JSON parsing
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
- 🟡 **Yellow**: Target UNKNOWN (timeout/connection issues)

## 🔍 Network Monitoring

### Enhanced Hybrid Scanning
- **PING**: Basic HTTP GET requests (10s timeout)
- **Health Check**: API endpoint verification with JSON parsing
- **Sequential**: One target at a time for stability
- **30s intervals**: Optimized for 24/7 operation
- **UNKNOWN Status**: Timeout scenarios marked as UNKNOWN
- **Smart Recovery**: Automatic retry in next scan cycle

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
│   │   ├── domain/
│   │   │   ├── alert/         # Alert management
│   │   │   ├── status/        # Status definitions
│   │   │   └── target/        # Target management
│   │   └── infrastructure/
│   │       ├── http_client/   # HTTP operations
│   │       ├── memory_manager/ # Memory management
│   │       ├── ntp_service/   # NTP synchronization
│   │       ├── telegram_service/ # Telegram alerts
│   │       ├── wifi_service/  # WiFi management
│   │       ├── ssl_mutex_manager/ # SSL thread safety
│   │       └── sdcard_manager/ # SD card operations
│   ├── tasks/
│   │   └── task_manager/      # FreeRTOS task management
│   ├── ui/
│   │   ├── display_manager/   # TFT display management
│   │   ├── touch_handler/     # Touch input handling
│   │   └── led_controller/    # RGB LED control
│   └── config/
│       └── config_loader/     # Configuration management
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
- **HTTP Timeout**: 10 seconds (intelligent timeout management)
- **Sequential Scanning**: One target at a time for stability
- **Alert Cooldown**: 5 minutes between alerts
- **UNKNOWN Status**: Timeout scenarios marked as UNKNOWN
- **Smart Recovery**: Automatic retry in next scan cycle

### Advanced Memory Management
- **Intelligent Garbage Collection**: Deferred during active scans
- **String Pool**: 10-string pool for optimization
- **Watchdog Bypass**: Full application control
- **Heartbeat System**: Aggressive 500ms feeding
- **Memory Monitoring**: Real-time heap and stack tracking
- **Race Condition Prevention**: GC deferred during scans

### 24/7 Stability
- **Tested**: Continuous operation without reboots
- **Memory Leaks**: Prevented by intelligent garbage collection
- **SSL Safety**: Thread-safe operations with mutex
- **Error Handling**: Robust error recovery mechanisms
- **Watchdog Bypass**: Full application control without system interference
- **Intelligent Timeouts**: Prevents system hangs on slow connections
- **Automatic Alert Reset**: Clean state for consistent behavior

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