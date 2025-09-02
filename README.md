# 🌌 Nebula Monitor v2.0

> **ESP32 TFT Network Monitor Dashboard** - A comprehensive network monitoring solution with touch interface

[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32-blue.svg)](https://platformio.org/)
[![LVGL](https://img.shields.io/badge/LVGL-8.3.11-green.svg)](https://lvgl.io/)
[![TFT_eSPI](https://img.shields.io/badge/TFT_eSPI-Latest-orange.svg)](https://github.com/Bodmer/TFT_eSPI)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![GitHub](https://img.shields.io/badge/GitHub-Tech--Tweakers-black.svg)](https://github.com/Tech-Tweakers)
[![Repository](https://img.shields.io/badge/Repo-nebula--monitor-blue.svg)](https://github.com/Tech-Tweakers/nebula-monitor)

## 📋 Table of Contents

- [🎯 Overview](#-overview)
- [✨ Features](#-features)
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

**Nebula Monitor** is a sophisticated network monitoring dashboard designed for ESP32-based TFT displays. It provides real-time monitoring of network services, servers, and endpoints with an intuitive touch interface powered by LVGL.

### 📍 Repository Information
- **GitHub**: [Tech-Tweakers/nebula-monitor](https://github.com/Tech-Tweakers/nebula-monitor)
- **Organization**: [Tech-Tweakers](https://github.com/Tech-Tweakers)
- **License**: MIT License
- **Status**: Active Development
- **Current Version**: v2.0
- **Releases**: [View Releases](https://github.com/Tech-Tweakers/nebula-monitor/releases)

### 🎪 Key Highlights

- **Real-time Network Monitoring**: Continuous health checks of multiple network targets
- **Touch Interface**: Interactive LVGL-based UI with responsive touch controls
- **WiFi Management**: Automatic connection and reconnection handling
- **Visual Status Indicators**: Color-coded status display with latency information
- **Uptime Tracking**: System uptime and service statistics
- **Professional UI**: Clean, modern interface with proper color mapping

## ✨ Features

### 🌐 Network Monitoring
- **Multi-target Scanning**: Monitor up to 6 network endpoints simultaneously
- **HTTP/HTTPS Support**: Full support for both HTTP and HTTPS protocols
- **Latency Measurement**: Real-time response time tracking
- **Status Classification**: UP/DOWN/UNKNOWN status with visual indicators
- **Automatic Retry**: Intelligent retry logic for failed connections

### 🎨 User Interface
- **LVGL Integration**: Modern, responsive UI framework
- **Touch Controls**: Full touch support with visual feedback
- **Color-coded Status**: Intuitive color system for quick status recognition
- **Dynamic Updates**: Real-time UI updates without flickering
- **Footer Toggle**: Switch between network info and system status

### 🔧 System Features
- **WiFi Auto-reconnect**: Automatic WiFi reconnection on failure
- **Memory Efficient**: Optimized memory usage for ESP32
- **Non-blocking Operations**: Asynchronous network operations
- **Debug Logging**: Comprehensive serial logging for troubleshooting

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

### 3️⃣ Configure WiFi
Edit `include/config.hpp`:
```cpp
#define WIFI_SSID "YourWiFiName"
#define WIFI_PASS "YourWiFiPassword"
```

### 4️⃣ Configure Network Targets
Edit `src/main.cpp` to add your monitoring targets:
```cpp
const Target targets[] = {
  {"Server 1", "http://192.168.1.100", UNKNOWN, 0},
  {"API Endpoint", "https://api.example.com", UNKNOWN, 0},
  {"Web Service", "http://192.168.1.200:8080", UNKNOWN, 0},
  // Add more targets as needed
};
```

### 5️⃣ Build and Upload
```bash
# Build the project
pio run

# Upload to ESP32
pio run --target upload

# Monitor serial output
pio device monitor
```

### 6️⃣ First Git Setup (Optional)
```bash
# Initialize git repository (if not already done)
git init

# Add remote origin
git remote add origin https://github.com/Tech-Tweakers/nebula-monitor.git

# Add all files
git add .

# Commit changes
git commit -m "Initial commit: Nebula Monitor v2.0"

# Push to repository
git push -u origin main
```

## ⚙️ Configuration

### 🌐 Network Settings

#### WiFi Configuration
```cpp
// In include/config.hpp
#define WIFI_SSID "YourNetworkName"
#define WIFI_PASS "YourPassword"
```

#### Target Configuration
```cpp
// In src/main.cpp
const Target targets[] = {
  {"Display Name", "http://target-url", UNKNOWN, 0},
  // name: Display name in UI
  // url: Full URL to monitor
  // st: Initial status (UNKNOWN)
  // lat_ms: Initial latency (0)
};
```

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

## 🎨 Color System

### 🔄 Color Inversion
This project uses a **color-inverted display** (ST7789 with `TFT_INVERSION_ON=1`). All colors are automatically inverted by the hardware.

### 🎯 Working Color Palette

| Desired Color | Code to Use | Result |
|---------------|-------------|---------|
| 🔴 **RED** | `0x00FFFF` | Status DOWN |
| 🟢 **GREEN** | `0xFF00FF` | Status UP (Good) |
| 🔵 **BLUE** | `0xFFFF00` | Status UP (Slow) |
| ⚪ **WHITE** | `0x000000` | Background |
| 🔘 **DARK GRAY** | `0x2d2d2d` | Title Bar |
| 🔘 **LIGHT GRAY** | `0x111111` | List Items |

### 📖 Color Reference
See [COLOR_MAPPING.md](COLOR_MAPPING.md) for detailed color mapping documentation.

## 📱 User Interface

### 🏠 Main Screen Layout

```
┌─────────────────────────────────┐
│        Nebula Monitor v3.0      │ ← Title Bar
├─────────────────────────────────┤
│ ┌─────────────────────────────┐ │
│ │ Target 1        [123 ms]    │ │ ← Status Items
│ │ Target 2        [DOWN]      │ │
│ │ Target 3        [456 ms]    │ │
│ │ Target 4        [789 ms]    │ │
│ │ Target 5        [DOWN]      │ │
│ │ Target 6        [234 ms]    │ │
│ └─────────────────────────────┘ │
├─────────────────────────────────┤
│ ● WiFi: OK | IP: 192.168.1.100 │ ← Footer (Clickable)
└─────────────────────────────────┘
```

### 🎮 Touch Interactions

#### Footer Toggle
- **Short Press**: Toggle between network info and system status
- **Network Mode**: Shows WiFi status and IP address
- **Status Mode**: Shows uptime and target statistics

#### Status Items
- **Touch**: Visual feedback with random color flash
- **Colors**: 
  - 🟢 Green: Target UP with good latency (<500ms)
  - 🔵 Blue: Target UP with high latency (≥500ms)
  - 🔴 Red: Target DOWN

### 📊 Status Indicators

#### Target Status
- **UP**: Service is responding
- **DOWN**: Service is not responding
- **UNKNOWN**: Initial state or error

#### Latency Display
- **Good**: <500ms (Green background)
- **Slow**: ≥500ms (Blue background)
- **Down**: "DOWN" text (Red background)

## 🔍 Network Monitoring

### 🔄 Scanning Process

1. **Initialization**: Scanner starts with all targets in UNKNOWN state
2. **Sequential Scanning**: Each target is checked in sequence
3. **HTTP Request**: GET request with 5-second timeout
4. **Response Analysis**: Status code and response time recorded
5. **UI Update**: Display updated with new status and latency
6. **Cycle Repeat**: Process repeats every 5 seconds

### 🌐 Supported Protocols

#### HTTP
- Standard HTTP requests
- Follows redirects
- Custom User-Agent: "NebulaWatch/1.0"

#### HTTPS
- Secure HTTPS requests
- Insecure mode (no certificate validation)
- Extended timeout for slow services (7+ seconds)

### ⚡ Performance Optimization

#### Timeout Management
```cpp
// Standard targets: 5 second timeout
// Cloudflare/Ngrok: 7+ second timeout (slower services)
```

#### Memory Management
- Non-blocking HTTP requests
- Efficient string handling
- Minimal memory allocation

## 🛠️ Development

### 🔧 Development Setup

#### Prerequisites
- PlatformIO Core or PlatformIO IDE
- ESP32 development board
- USB cable for programming
- Serial monitor access

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
[MAIN] Iniciando Nebula Monitor v3.0...
[MAIN] Conectando ao WiFi...
[MAIN] WiFi conectado com sucesso!
[MAIN] Display inicializado com sucesso!
[MAIN] Touch inicializado com sucesso!
[MAIN] Scanner inicializado com sucesso!
[SCANNER] Target 0: Status=1, Latency=123
[TOUCH] Touch detectado em (120, 160)
```

#### Common Issues
1. **WiFi Connection Failed**: Check SSID/password in config.hpp
2. **Display Not Working**: Verify pin connections and TFT driver settings
3. **Touch Not Responding**: Check touch calibration values
4. **Network Timeouts**: Verify target URLs and network connectivity

### 🔄 Code Structure

#### Main Components
- **main.cpp**: Application entry point and main loop
- **display.cpp**: TFT display management and LVGL integration
- **touch.cpp**: Touch input handling and calibration
- **net.cpp**: WiFi and HTTP client functionality
- **scan.cpp**: Network monitoring and target management

#### Key Classes
- **DisplayManager**: TFT display and LVGL management
- **Touch**: Touch input handling
- **Net**: WiFi and HTTP operations
- **ScanManager**: Network monitoring logic

## 📁 Project Structure

```
nebula-monitor/
├── 📁 data/                    # Asset files
│   └── tt_logo.png            # Logo image
├── 📁 include/                 # Header files
│   ├── config.hpp             # Configuration constants
│   ├── display.hpp            # Display manager interface
│   ├── lv_conf.h              # LVGL configuration
│   ├── net.hpp                # Network operations interface
│   ├── touch.hpp              # Touch input interface
│   └── User_Setup.h           # TFT_eSPI user setup
├── 📁 lib/                     # Library files
│   └── README                 # Library documentation
├── 📁 src/                     # Source files
│   ├── main.cpp               # Main application
│   ├── display.cpp            # Display implementation
│   ├── net.cpp                # Network implementation
│   ├── scan.cpp               # Scanner implementation
│   ├── scan.hpp               # Scanner interface
│   └── touch.cpp              # Touch implementation
├── 📁 test/                    # Test files
│   └── README                 # Test documentation
├── 📄 platformio.ini          # PlatformIO configuration
├── 📄 COLOR_MAPPING.md        # Color system documentation
└── 📄 README.md               # This file
```

## 🐛 Troubleshooting

### ❌ Common Problems

#### WiFi Issues
```
Problem: WiFi connection fails
Solution: 
1. Check SSID/password in config.hpp
2. Verify WiFi network is 2.4GHz (ESP32 doesn't support 5GHz)
3. Check signal strength
4. Try different WiFi credentials
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
```

#### Network Monitoring Issues
```
Problem: Targets always show DOWN
Solution:
1. Verify target URLs are accessible
2. Check network connectivity
3. Increase timeout values for slow services
4. Verify HTTP vs HTTPS protocol usage
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
- `[TOUCH]`: Touch input events
- `[FOOTER]`: Footer interaction events

## 📈 Performance

### ⚡ System Performance

#### Memory Usage
- **Flash**: ~1.2MB (typical ESP32 project)
- **RAM**: ~200KB (with LVGL buffers)
- **Free Heap**: ~150KB (available for operations)

#### Network Performance
- **Scan Interval**: 5 seconds per cycle
- **HTTP Timeout**: 5 seconds (standard), 7+ seconds (cloud services)
- **Concurrent Requests**: 1 (sequential scanning)
- **Memory per Request**: ~2KB

#### Display Performance
- **Refresh Rate**: 60 FPS (LVGL default)
- **Touch Response**: <50ms
- **UI Updates**: Non-blocking, smooth animations

### 🎯 Optimization Tips

#### Memory Optimization
1. Use `const` for static strings
2. Minimize string concatenation
3. Use stack allocation when possible
4. Monitor heap usage with `ESP.getFreeHeap()`

#### Network Optimization
1. Use appropriate timeouts for each service
2. Implement connection pooling
3. Cache DNS lookups
4. Use HTTP/2 when available

#### Display Optimization
1. Minimize LVGL object creation
2. Use efficient color formats
3. Implement dirty region updates
4. Optimize touch event handling

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

---

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/Tech-Tweakers/nebula-monitor/issues)
- **Discussions**: [GitHub Discussions](https://github.com/Tech-Tweakers/nebula-monitor/discussions)
- **Documentation**: [Project Wiki](https://github.com/Tech-Tweakers/nebula-monitor/wiki)
- **Organization**: [Tech-Tweakers](https://github.com/Tech-Tweakers)

---

<div align="center">

**🌌 Nebula Monitor v2.0** - *Monitoring the digital cosmos, one packet at a time*

Made with ❤️ for the ESP32 community

</div>
