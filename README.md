# 🌌 Nebula Monitor v2.3

> **ESP32 TFT Network Monitor Dashboard** - A comprehensive network monitoring solution with external configuration, Telegram alerts, hybrid monitoring, and intelligent touch interface

[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32-blue.svg)](https://platformio.org/)
[![LVGL](https://img.shields.io/badge/LVGL-8.3.11-green.svg)](https://lvgl.io/)
[![TFT_eSPI](https://img.shields.io/badge/TFT_eSPI-Latest-orange.svg)](https://github.com/Bodmer/TFT_eSPI)
[![Telegram](https://img.shields.io/badge/Telegram-Alerts-blue.svg)](https://telegram.org/)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![GitHub](https://img.shields.io/badge/GitHub-Tech--Tweakers-black.svg)](https://github.com/Tech-Tweakers)
[![Repository](https://img.shields.io/badge/Repo-nebula--monitor-blue.svg)](https://github.com/Tech-Tweakers/nebula-monitor)

## 📋 Table of Contents

- [🎯 Overview](#-overview)
- [✨ Features](#-features)
- [🚨 Telegram Alerts](#-telegram-alerts)
- [🔧 Hardware Requirements](#-hardware-requirements)
- [📦 Dependencies](#-dependencies)
- [🚀 Quick Start](#-quick-start)
- [⚙️ Configuration](#️-configuration)
- [🎨 Color System](#-color-system)
- [📱 User Interface](#-user-interface)
- [🔍 Network Monitoring](#-network-monitoring)
- [🛠️ Development](#️-development)
- [📁 Project Structure](#-project-structure)
- [🐛 Troubleshooting](#-troubleshooting)
- [📈 Performance](#-performance)
- [🤝 Contributing](#-contributing)
- [📄 License](#-license)

## 🎯 Overview

**Nebula Monitor v2.3** is a sophisticated network monitoring dashboard designed for ESP32-based TFT displays. It provides real-time monitoring of network services, servers, and endpoints with an intuitive touch interface powered by LVGL, complete with external configuration management, Telegram alert system, and hybrid monitoring capabilities.

### 📍 Repository Information
- **GitHub**: [Tech-Tweakers/nebula-monitor](https://github.com/Tech-Tweakers/nebula-monitor)
- **Organization**: [Tech-Tweakers](https://github.com/Tech-Tweakers)
- **License**: MIT License
- **Status**: Active Development
- **Current Version**: v2.3
- **Releases**: [View Releases](https://github.com/Tech-Tweakers/nebula-monitor/releases)

### 🎪 Key Highlights

- **📁 External Configuration**: All settings managed via `config.env` file on SPIFFS
- **🚨 Telegram Alert System**: Real-time notifications for service outages and recoveries
- **🔄 Hybrid Monitoring**: PING for basic connectivity + Health Check for API endpoints
- **📊 Intelligent Footer**: 5 modes of system information with compact display (centered text)
- **🔵🔴🟢 Hardware Status LED**: Primary-color LED shows system status (Blue scan/Telegram, Red DOWN, Green OK)
- **⚡ Performance Optimized**: 30s scan intervals, conditional logging, touch optimized
- **🎮 Touch Interface**: Interactive LVGL-based UI with responsive touch controls
- **🌐 WiFi Management**: Automatic connection and reconnection handling
- **📈 Uptime Tracking**: System uptime and service statistics
- **🔧 Runtime Configuration**: Debug settings and monitoring parameters configurable at runtime

## ✨ Features

### 📁 External Configuration System
- **SPIFFS Integration**: All configuration stored in `data/config.env` file
- **Runtime Configuration**: No need to recompile for configuration changes
- **Comprehensive Settings**: WiFi, Telegram, targets, display, touch, and debug settings
- **Easy Management**: Simple key-value format for all configuration parameters
- **Fallback Support**: Hardcoded defaults if configuration file is missing
- **Dynamic Loading**: Targets loaded dynamically from configuration file
- **Memory Efficient**: Uses `strdup()` for dynamic string allocation

### 🚨 Telegram Alert System
- **Real-time Notifications**: Instant alerts when services go down or recover
- **Smart Thresholds**: Configurable failure count before alerting (default: 3 failures)
- **Cooldown Management**: Prevents spam with 5-minute alert cooldowns
- **Recovery Alerts**: Notifications when services come back online
- **Downtime Tracking**: Total downtime duration in recovery messages
- **Auto Chat ID Discovery**: Automatic Telegram chat ID detection
- **Rich Message Format**: Formatted alerts with emojis and detailed information

### 🔄 Hybrid Network Monitoring
- **PING Monitoring**: Basic connectivity checks for routers and servers
- **Health Check Monitoring**: API endpoint status verification with JSON payload parsing
- **Multi-target Scanning**: Monitor up to 6 network endpoints simultaneously
- **HTTP/HTTPS Support**: Full support for both HTTP and HTTPS protocols
- **Latency Measurement**: Real-time response time tracking
- **Status Classification**: UP/DOWN/UNKNOWN status with visual indicators
- **Automatic Retry**: Intelligent retry logic for failed connections

### 🎨 Intelligent User Interface
- **LVGL Integration**: Modern, responsive UI framework
- **Touch Controls**: Full touch support with visual feedback
- **Color-coded Status**: Intuitive color system for quick status recognition
- **Dynamic Updates**: Real-time UI updates without flickering
- **Smart Footer**: 5 modes of system information (System, Network, Performance, Targets, Uptime) with centered text
- **LED Synchronization**: Screen, footer and LED update in lockstep at scan start/finish
- **Compact Display**: Abbreviated text for maximum information density

### 🔧 Advanced System Features
- **WiFi Auto-reconnect**: Automatic WiFi reconnection on failure
- **Memory Efficient**: Optimized memory usage for ESP32
- **Non-blocking Operations**: Asynchronous network operations
- **Conditional Logging**: Debug logs can be enabled/disabled for performance
- **Touch Optimization**: 10ms delay for improved touch responsiveness
- **Scan Management**: Intelligent scan state management with proper start/stop
 - **Scan Callbacks**: Start/complete callbacks synchronize UI refresh, footer, and LED in the same cycle

## 🚨 Telegram Alerts

### 📱 Alert Configuration

#### Bot Setup
1. Create a Telegram bot via [@BotFather](https://t.me/botfather)
2. Get your bot token
3. Configure in `include/config.hpp`:
```cpp
#define TELEGRAM_BOT_TOKEN "your_bot_token_here"
#define TELEGRAM_CHAT_ID "your_chat_id_here"
#define TELEGRAM_ENABLED true
```

#### Alert Settings
```cpp
#define MAX_FAILURES_BEFORE_ALERT 3        // Failures before alert
#define ALERT_COOLDOWN_MS 300000           // 5 minutes between alerts
#define ALERT_RECOVERY_COOLDOWN_MS 60000   // 1 minute for recovery alerts
```

### 🔔 Alert Types

#### Service Down Alert
```
🔴 ALERTA - Proxmox HV

❌ Proxmox HV está com problemas!
📊 Status: DOWN
⏱️ Última latência: 0 ms
🕐 00:05:23 de downtime

🌌 Nebula Monitor v2.2
```

#### Service Recovery Alert
```
🟢 ONLINE - Proxmox HV

✅ Proxmox HV está funcionando novamente!
⏱️ Latência: 45 ms
🕐 01:23:45 de uptime
⏰ Downtime total: 00:05:23

🌌 Nebula Monitor v2.2
```

#### System Startup Alert
```
🚀 Nebula Monitor v2.2 iniciado com sucesso!

✅ Sistema de alertas ativo
📊 Monitorando 6 targets
🔔 Threshold: 3 falhas
⏰ Cooldown: 300s

Tech Tweakers
```

## 🔧 Hardware Requirements

### 📱 Display Module
- **Board**: ESP32 Development Board (ESP32-DevKitC or similar)
- **TFT Display**: ST7789 Driver (240x320 resolution)
- **Touch Controller**: XPT2046 Resistive Touch
- **Backlight**: PWM-controlled backlight (GPIO 27)

### 🔌 Pin Configuration

| Component | ESP32 Pin | Function |
|-----------|-----------|----------|
| **TFT_MOSI** | GPIO 13 | SPI Data |
| **TFT_SCLK** | GPIO 14 | SPI Clock |
| **TFT_MISO** | GPIO 12 | SPI Data (Input) |
| **TFT_CS** | GPIO 15 | Chip Select |
| **TFT_DC** | GPIO 2 | Data/Command |
| **TFT_RST** | -1 | Reset (Hardware) |
| **TFT_BL** | GPIO 27 | Backlight Control |
| **TOUCH_CS** | GPIO 33 | Touch Chip Select |
| **TOUCH_IRQ** | GPIO 36 | Touch Interrupt |

### 🎯 Recommended Hardware
- **Cheap Yellow Board (CYB)**: Pre-configured ESP32 + TFT module
- **Alternative**: Any ESP32 board with ST7789 TFT and XPT2046 touch

## 📦 Dependencies

### 📚 Core Libraries
```ini
lib_deps =
  TFT_eSPI                    # TFT display driver
  https://github.com/PaulStoffregen/XPT2046_Touchscreen.git  # Touch controller
  lvgl/lvgl@^8.3.11          # UI framework
```

### 🔧 Build Configuration
- **Platform**: ESP32 (Espressif32)
- **Framework**: Arduino
- **Monitor Speed**: 115200 baud
- **SPI Frequency**: 40MHz (Display), 2.5MHz (Touch)

## 🚀 Quick Start

### 1️⃣ Clone the Repository
```bash
git clone https://github.com/Tech-Tweakers/nebula-monitor.git
cd nebula-monitor
```

### 2️⃣ Install PlatformIO
```bash
# Install PlatformIO Core
pip install platformio

# Or use PlatformIO IDE
# Download from: https://platformio.org/platformio-ide
```

### 3️⃣ Configure System Settings
Edit `data/config.env` file:
```env
# WiFi Configuration
WIFI_SSID=YourWiFiName
WIFI_PASS=YourWiFiPassword

# Telegram Configuration
TELEGRAM_BOT_TOKEN=your_bot_token_here
TELEGRAM_CHAT_ID=your_chat_id_here
TELEGRAM_ENABLED=true

# Alert Settings
MAX_FAILURES_BEFORE_ALERT=3
ALERT_COOLDOWN_MS=300000
ALERT_RECOVERY_COOLDOWN_MS=60000

# Debug Settings
DEBUG_LOGS_ENABLED=false
TOUCH_LOGS_ENABLED=false
ALL_LOGS_ENABLED=false

# Network Targets (Format: NAME|URL|HEALTH_ENDPOINT|MONITOR_TYPE)
TARGET_1=Proxmox HV|http://192.168.1.128:8006/||PING
TARGET_2=Router #1|http://192.168.1.1||PING
TARGET_3=Router #2|https://192.168.1.172||PING
TARGET_4=Polaris API|https://api.example.com|/health|HEALTH_CHECK
TARGET_5=Polaris INT|http://integration.example.com|/health|HEALTH_CHECK
TARGET_6=Polaris WEB|https://web.example.com|/#/health|HEALTH_CHECK

# LED (RGB Status)
# Primary colors only: BLUE (scan/telegram), RED (down), GREEN (ok)
LED_PIN_R=16
LED_PIN_G=17
LED_PIN_B=20
LED_ACTIVE_HIGH=false
LED_PWM_FREQ=5000
LED_PWM_RES_BITS=8
LED_BRIGHT_R=32
LED_BRIGHT_G=12
LED_BRIGHT_B=12
```

### 4️⃣ Upload Configuration and Firmware
```bash
# Upload configuration file to SPIFFS
pio run --target uploadfs

# Build and upload firmware
pio run --target upload

# Monitor serial output
pio device monitor
```

## ⚙️ Configuration

### 📁 External Configuration System

All configuration is now managed through the `data/config.env` file, which is uploaded to the ESP32's SPIFFS filesystem. This allows for runtime configuration changes without recompiling the firmware.

#### Configuration File Structure
```env
# WiFi Settings
WIFI_SSID=YourNetworkName
WIFI_PASS=YourPassword

# Telegram Bot Configuration
TELEGRAM_BOT_TOKEN=your_bot_token_here
TELEGRAM_CHAT_ID=your_chat_id_here
TELEGRAM_ENABLED=true

# Alert Configuration
MAX_FAILURES_BEFORE_ALERT=3
ALERT_COOLDOWN_MS=300000
ALERT_RECOVERY_COOLDOWN_MS=60000

# Debug Settings (Runtime Configurable)
DEBUG_LOGS_ENABLED=false
TOUCH_LOGS_ENABLED=false
ALL_LOGS_ENABLED=false

# Display Settings
DISPLAY_ROTATION=0
BACKLIGHT_PIN=27

# Touch Settings
TOUCH_SCK_PIN=14
TOUCH_MOSI_PIN=13
TOUCH_MISO_PIN=12
TOUCH_CS_PIN=33
TOUCH_IRQ_PIN=36
TOUCH_X_MIN=200
TOUCH_X_MAX=3700
TOUCH_Y_MIN=240
TOUCH_Y_MAX=3800

# Scan Settings
SCAN_INTERVAL_MS=30000
TOUCH_FILTER_MS=500
HTTP_TIMEOUT_MS=5000

# Network Targets (Format: NAME|URL|HEALTH_ENDPOINT|MONITOR_TYPE)
TARGET_1=Proxmox HV|http://192.168.1.128:8006/||PING
TARGET_2=Router #1|http://192.168.1.1||PING
TARGET_3=Router #2|https://192.168.1.172||PING
TARGET_4=Polaris API|https://api.example.com|/health|HEALTH_CHECK
TARGET_5=Polaris INT|http://integration.example.com|/health|HEALTH_CHECK
TARGET_6=Polaris WEB|https://web.example.com|/#/health|HEALTH_CHECK
```

#### Target Configuration Format
```
TARGET_N=NAME|URL|HEALTH_ENDPOINT|MONITOR_TYPE
```
- **NAME**: Display name in the UI
- **URL**: Full URL to monitor (http:// or https://)
- **HEALTH_ENDPOINT**: Health check endpoint path (empty for PING)
- **MONITOR_TYPE**: `PING` or `HEALTH_CHECK`

### 🎨 Display Settings
#### TFT Configuration
```cpp
// In platformio.ini
build_flags =
  -D TFT_WIDTH=240
  -D TFT_HEIGHT=320
  -D TFT_RGB_ORDER=TFT_BGR
  -D TFT_INVERSION_ON=1
  -D TFT_BL=27
```

#### Touch Calibration
```cpp
// In include/config.hpp
static constexpr int RAW_X_MIN = 200;
static constexpr int RAW_X_MAX = 3700;
static constexpr int RAW_Y_MIN = 240;
static constexpr int RAW_Y_MAX = 3800;
```

### 🔧 Performance Settings
#### Debug Logging
```env
# In data/config.env
DEBUG_LOGS_ENABLED=false      # General debug logs
TOUCH_LOGS_ENABLED=false      # Touch-specific logs
ALL_LOGS_ENABLED=false        # Master switch
```

#### Scan Configuration
```cpp
// In src/main.cpp
static const unsigned long SCAN_INTERVAL = 30000; // 30 seconds between scans
```

## 🎨 Color System

### 🔄 Color Inversion
This project uses a color-inverted ST7789 display (`TFT_INVERSION_ON=1`). Colors in code are inverted by hardware.

### 🎯 Working Color Palette

| Desired Color | Code to Use | Result |
|---------------|-------------|--------|
| 🔴 RED        | `0x00FFFF`  | Down/alert |
| 🟢 GREEN      | `0xFF00FF`  | OK/UP |
| 🔵 BLUE       | `0xFFFF00`  | Slow/scan hint |
| ⚪ WHITE      | `0x000000`  | Background |
| 🔘 DARK GRAY  | `0x2d2d2d`  | Title Bar |
| 🔘 LIGHT GRAY | `0x111111`  | List Items |

## 📱 User Interface

### 🏠 Main Screen Layout

```
┌─────────────────────────────────┐
│        Nebula Monitor v2.2      │ ← Title Bar
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
│ 🟢 Sys: OK | Alt: 0 | 6/6 UP   │ ← Smart Footer (Clickable)
└─────────────────────────────────┘
```

### 🎮 Touch Interactions

#### Footer Toggle (5 Modes)
- **Touch**: Cycles through 5 information modes
- **Mode 0 - System**: `Sys: OK | Alt: 0 | 6/6 UP`
- **Mode 1 - Network**: `IP: 192.168.1.162 | -45 dBm | 30s`
- **Mode 2 - Performance**: `CPU: 45% | RAM: 32% | UP: 01:23`
- **Mode 3 - Targets**: `UP: 4 | DN: 2 | UNK: 0 | SCAN`
- **Mode 4 - Uptime**: `UP: 01:23 | RAM: 107KB | Next: 28s`

#### Status Items
- **Touch**: Visual feedback with random color flash
- **Colors**: 
  - 🟢 Green: Target UP with good latency (<500ms)
  - 🟠 Orange: Target UP with high latency (≥500ms)
  - 🔴 Red: Target DOWN

<!-- Visual Scan Indicator (bolinha) removed in favor of hardware LED status -->

### 📊 Status Indicators

#### Target Status
- **UP**: Service is responding
- **DOWN**: Service is not responding
- **UNKNOWN**: Initial state or error

#### Latency Display
- **Good**: <500ms (Green background)
- **Slow**: ≥500ms (Blue background)
- **Down**: "DOWN" text (Red background)
- **Health Check**: "OK" text (Green background)

## 🔍 Network Monitoring

### 🔄 Hybrid Scanning Process

1. **Initialization**: Scanner starts with all targets in UNKNOWN state
2. **Sequential Scanning**: Each target is checked in sequence
3. **Monitoring Type Selection**:
   - **PING**: Basic HTTP GET request with 5-second timeout
   - **HEALTH_CHECK**: API endpoint check with JSON payload parsing
4. **Response Analysis**: Status code, response time, and health status recorded
5. **UI Update**: Display updated with new status and latency
6. **Alert Processing**: Telegram alerts sent based on failure thresholds
7. **Cycle Repeat**: Process repeats every 30 seconds

### 🌐 Supported Protocols

#### HTTP PING
- Standard HTTP requests
- Follows redirects
- Custom User-Agent: "NebulaWatch/1.0"
- 5-second timeout

#### HTTPS PING
- Secure HTTPS requests
- Insecure mode (no certificate validation)
- Extended timeout for slow services (7+ seconds)

#### Health Check API
- JSON payload parsing
- Looks for `"status":"healthy"` in response
- Supports various API response formats
- Automatic HTTP fallback for HTTPS failures

### ⚡ Performance Optimization

#### Scan Management
```cpp
// 30-second scan intervals (vs 5 seconds in v2.0)
// Proper isScanning state management
// Touch optimization with 10ms delays
```

#### Memory Management
- Non-blocking HTTP requests
- Efficient string handling
- Minimal memory allocation
- Conditional logging system

## 🛠️ Development

### 🔧 Development Setup

#### Prerequisites
- PlatformIO Core or PlatformIO IDE
- ESP32 development board
- USB cable for programming
- Serial monitor access
- Telegram bot (for alert testing)

#### Build Commands
```bash
# Clean build
pio run --target clean

# Build project
pio run

# Upload to device
pio run --target upload

# Monitor serial output
pio device monitor

# Build and upload in one command
pio run --target upload
```

### 🐛 Debugging

#### Serial Output
The project provides comprehensive debug logging:
```
[MAIN] Iniciando Nebula Monitor v2.2...
[MAIN] Conectando ao WiFi...
[MAIN] WiFi conectado com sucesso!
[MAIN] Display inicializado com sucesso!
[MAIN] Touch inicializado com sucesso!
[MAIN] Scanner inicializado com sucesso!
[MAIN] Sistema de alertas Telegram inicializado!
[SCANNER] Target 0: Status=1, Latency=123
[TELEGRAM] Alerta enviado com sucesso!
[TOUCH] Touch detectado em (120, 160)
```

#### Debug Logging Control
```cpp
// Enable/disable debug logs in include/config.hpp
#define DEBUG_LOGS_ENABLED true      // General debug logs
#define TOUCH_LOGS_ENABLED true      // Touch-specific logs
```

#### Common Issues
1. **WiFi Connection Failed**: Check SSID/password in config.hpp
2. **Display Not Working**: Verify pin connections and TFT driver settings
3. **Touch Not Responding**: Check touch calibration values
4. **Network Timeouts**: Verify target URLs and network connectivity
5. **Telegram Alerts Not Working**: Check bot token and chat ID
6. **Touch Slow/Unresponsive**: Enable touch optimization, check scan frequency

### 🔄 Code Structure

#### Main Components
- **main.cpp**: Application entry point, UI management, and main loop
- **display.cpp**: TFT display management and LVGL integration
- **touch.cpp**: Touch input handling and calibration
- **net.cpp**: WiFi and HTTP client functionality
- **scan.cpp**: Network monitoring and target management
- **telegram.cpp**: **NEW** - Telegram alert system implementation

#### Key Classes
- **DisplayManager**: TFT display and LVGL management
- **Touch**: Touch input handling
- **Net**: WiFi and HTTP operations
- **ScanManager**: Network monitoring logic
- **TelegramAlerts**: **NEW** - Telegram notification system

## 📁 Project Structure

```
nebula-monitor/
├── 📁 include/                 # Header files
│   ├── config.hpp             # Configuration constants and runtime settings
│   ├── config_manager.hpp     # **NEW** - External configuration manager
│   ├── display.hpp            # Display manager interface
│   ├── lv_conf.h              # LVGL configuration
│   ├── net.hpp                # Network operations interface
│   ├── scan.hpp               # Scanner interface
│   ├── telegram.hpp           # Telegram alerts interface
│   └── touch.hpp              # Touch input interface
├── 📁 src/                     # Source files
│   ├── main.cpp               # Main application with external config integration
│   ├── config_manager.cpp     # **NEW** - Configuration manager implementation
│   ├── display.cpp            # Display implementation
│   ├── net.cpp                # Network implementation
│   ├── scan.cpp               # Scanner implementation
│   ├── telegram.cpp           # Telegram alerts implementation
│   └── touch.cpp              # Touch implementation
├── 📁 data/                    # **NEW** - SPIFFS data files
│   └── config.env             # **NEW** - External configuration file
├── 📁 test/                    # Test files
│   └── README                 # Test documentation
├── 📄 platformio.ini          # PlatformIO configuration with SPIFFS support
├── 📄 COLOR_MAPPING.md        # Color system documentation
└── 📄 README.md               # This file
```

## 🐛 Troubleshooting

### ❌ Common Problems

#### Configuration Issues
```
Problem: Configuration not loading or "black screen"
Solution:
1. Ensure config.env file exists in data/ directory
2. Upload SPIFFS filesystem: pio run --target uploadfs
3. Check config.env file format (no spaces around =)
4. Verify all required parameters are present
5. Check serial output for configuration loading errors
```

#### WiFi Issues
```
Problem: WiFi connection fails
Solution: 
1. Check SSID/password in data/config.env
2. Verify WiFi network is 2.4GHz (ESP32 doesn't support 5GHz)
3. Check signal strength
4. Try different WiFi credentials
5. Ensure config file was uploaded to SPIFFS
```

#### Display Issues
```
Problem: Display shows nothing or garbled content
Solution:
1. Verify pin connections
2. Check TFT driver settings in platformio.ini
3. Ensure TFT_INVERSION_ON=1 is set
4. Verify backlight pin (GPIO 27)
```

#### Touch Issues
```
Problem: Touch not responding or inaccurate
Solution:
1. Check touch calibration values in config.hpp
2. Verify TOUCH_CS pin (GPIO 33)
3. Recalibrate touch coordinates
4. Check touch controller wiring
5. Enable touch optimization (10ms delay)
6. Check scan frequency (30s intervals)
```

#### Network Monitoring Issues
```
Problem: Targets always show DOWN
Solution:
1. Verify target URLs are accessible
2. Check network connectivity
3. Increase timeout values for slow services
4. Verify HTTP vs HTTPS protocol usage
5. Check health check endpoint configuration
6. Verify JSON payload format for health checks
```

#### Telegram Alert Issues
```
Problem: Telegram alerts not working
Solution:
1. Verify bot token in config.hpp
2. Check chat ID configuration
3. Send a message to the bot first
4. Verify TELEGRAM_ENABLED is true
5. Check network connectivity
6. Monitor serial output for Telegram errors
```

### 🔍 Debug Information

#### Serial Monitor Output
Enable serial monitoring to see debug information:
```bash
pio device monitor --baud 115200
```

#### Key Debug Messages
- `[MAIN]`: Application startup and initialization
- `[NET]`: WiFi and network operations
- `[SCANNER]`: Network monitoring status
- `[TELEGRAM]`: **NEW** - Telegram alert operations
- `[TOUCH]`: Touch input events
- `[FOOTER]`: Footer interaction events

## 📈 Performance

### ⚡ System Performance

#### Memory Usage
- **Flash**: ~1.17MB (89.3% of 1.31MB)
- **RAM**: ~108KB (32.9% of 327KB)
- **Free Heap**: ~220KB (available for operations)

#### Network Performance
- **Scan Interval**: 30 seconds per cycle (optimized from 5s)
- **HTTP Timeout**: 5 seconds (standard), 7+ seconds (cloud services)
- **Concurrent Requests**: 1 (sequential scanning)
- **Memory per Request**: ~2KB
- **Alert Cooldown**: 5 minutes between alerts

#### Display Performance
- **Refresh Rate**: 60 FPS (LVGL default)
- **Touch Response**: <50ms (optimized with 10ms delay)
- **UI Updates**: Non-blocking, smooth animations
- **Footer Updates**: 500ms intervals

### 🎯 Optimization Features

#### Performance Optimizations
1. **Conditional Logging**: Debug logs can be disabled for production
2. **Scan Frequency**: Reduced from 5s to 30s for better performance
3. **Touch Optimization**: 10ms delay for improved responsiveness
4. **Memory Management**: Efficient string handling and minimal allocation
5. **State Management**: Proper scan state tracking

#### Network Optimizations
1. **Hybrid Monitoring**: PING for basic, Health Check for APIs
2. **Intelligent Timeouts**: Different timeouts for different service types
3. **HTTP Fallback**: Automatic HTTP fallback for HTTPS failures
4. **Connection Reuse**: Efficient HTTP client management

#### UI Optimizations
1. **Compact Footer**: Abbreviated text for maximum information
2. **Visual Indicators**: Bolinha verde/vermelha for scan status
3. **Smart Updates**: Only update when necessary
4. **Touch Feedback**: Visual feedback for all interactions

## 🤝 Contributing

### 🔄 Contribution Guidelines

1. **Fork the repository**: [Fork nebula-monitor](https://github.com/Tech-Tweakers/nebula-monitor/fork)
2. **Create a feature branch**: `git checkout -b feature/amazing-feature`
3. **Commit changes**: `git commit -m 'Add amazing feature'`
4. **Push to branch**: `git push origin feature/amazing-feature`
5. **Open a Pull Request**: [Create PR](https://github.com/Tech-Tweakers/nebula-monitor/compare)

### 🎯 Contribution Areas

- **Hardware Support**: Add support for new TFT displays or touch controllers
- **Network Features**: Implement new monitoring protocols or features
- **UI/UX Improvements**: Enhance the LVGL interface and user experience
- **Performance**: Optimize memory usage and network operations
- **Telegram Features**: Enhance alert system with new features
- **Documentation**: Improve code comments and documentation
- **Testing**: Add unit tests and integration tests

### 📝 Code Style

#### C++ Style Guidelines
- Use `const` for immutable values
- Prefer `constexpr` for compile-time constants
- Use meaningful variable names
- Add comments for complex logic
- Follow Arduino/ESP32 conventions

#### File Organization
- Keep header files in `include/`
- Keep implementation files in `src/`
- Use descriptive file names
- Group related functionality

### 🐛 Bug Reports

When reporting bugs, please include:
1. **Hardware configuration** (board, display model)
2. **Software version** (PlatformIO, library versions)
3. **Error messages** (serial output)
4. **Steps to reproduce**
5. **Expected vs actual behavior**
6. **Telegram configuration** (if applicable)

### 💡 Feature Requests

For feature requests, please include:
1. **Use case description**
2. **Proposed implementation**
3. **Benefits to users**
4. **Potential challenges**

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

### 📋 License Summary

- ✅ **Commercial use** allowed
- ✅ **Modification** allowed
- ✅ **Distribution** allowed
- ✅ **Private use** allowed
- ❌ **Liability** not provided
- ❌ **Warranty** not provided

---

## 🎉 Acknowledgments

- **LVGL Community** for the excellent UI framework
- **TFT_eSPI Library** for robust display support
- **ESP32 Community** for comprehensive documentation
- **PlatformIO** for the excellent development platform
- **Telegram** for the powerful bot API

---

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/Tech-Tweakers/nebula-monitor/issues)
- **Discussions**: [GitHub Discussions](https://github.com/Tech-Tweakers/nebula-monitor/discussions)
- **Documentation**: [Project Wiki](https://github.com/Tech-Tweakers/nebula-monitor/wiki)
- **Organization**: [Tech-Tweakers](https://github.com/Tech-Tweakers)

---

<div align="center">

**🌌 Nebula Monitor v2.3** - *Monitoring the digital cosmos with external configuration, Telegram alerts, and hybrid intelligence*

Made with ❤️ for the ESP32 community

**🚀 Now with External Configuration, Telegram Alerts, Hybrid Monitoring, and Intelligent Touch Interface!**

</div>