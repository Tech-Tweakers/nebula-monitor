# Nebula Monitor v2.4

> **ESP32 TFT Network Monitor Dashboard** — Production-ready network monitoring with GitHub dark UI, group-based pagination, per-target detail modals, Telegram alerts, and 24/7 stability.

[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32-blue.svg)](https://platformio.org/)
[![TFT_eSPI](https://img.shields.io/badge/TFT__eSPI-raw-orange.svg)](https://github.com/Bodmer/TFT_eSPI)
[![Telegram](https://img.shields.io/badge/Telegram-Alerts-blue.svg)](https://telegram.org/)
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/Tech-Tweakers/nebula-monitor)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![GitHub](https://img.shields.io/badge/GitHub-Tech--Tweakers-black.svg)](https://github.com/Tech-Tweakers)

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Quick Start](#quick-start)
- [Configuration](#configuration)
- [User Interface](#user-interface)
- [Development](#development)
- [Project Structure](#project-structure)
- [Troubleshooting](#troubleshooting)
- [Performance](#performance)

## Overview

**Nebula Monitor v2.4** is a production-ready network monitoring dashboard for ESP32 TFT displays. The UI is built entirely with raw **TFT_eSPI** (no LVGL) for maximum performance and stability. It monitors up to 30 targets organized in named groups, shows per-target detail modals on tap, and sends smart Telegram alerts with cooldown management.

### Key Features

- **Raw TFT_eSPI UI**: No LVGL — GitHub dark palette, smooth rendering, 84% flash / 15% RAM
- **Group-based pagination**: Targets organized by group, navigate pages by tapping the title bar
- **Per-target modal**: Tap any target to see status, latency, fail count, last down duration, time since last change
- **Dynamic footer**: 3 tap-to-cycle modes — system overview, WiFi signal, memory stats
- **RGB LED indicator**: Visual system status at a glance
- **Telegram alerts**: Smart notifications with failure threshold and cooldown management
- **Hybrid monitoring**: PING + Health Check modes per target
- **SD card sync**: Boot-time config sync from SD to SPIFFS with smart timestamp/hash comparison
- **24/7 stability**: FreeRTOS dual-core, watchdog feeding, pending-refresh pattern to prevent cross-core crashes

## Features

### UI Architecture (raw TFT_eSPI)

LVGL was removed entirely in v2.4. All rendering uses direct TFT_eSPI calls with a GitHub dark palette. This cut flash usage from 98% → 84% and RAM from 33% → 15%.

**Color inversion quirk** — this display requires bitwise NOT on all color565 values due to `TFT_INVERSION_ON` + `TFT_BGR` combined effect. All colors pass through the `c()` helper:

```cpp
static inline uint16_t c(uint32_t rgb) {
  uint8_t r = (rgb >> 16) & 0xFF;
  uint8_t g = (rgb >> 8)  & 0xFF;
  uint8_t b =  rgb        & 0xFF;
  return ~tft->color565(r, g, b);
}
```

Pass standard `0xRRGGBB` hex values — the helper handles inversion automatically.

### Group-based Pagination

Targets are assigned to named groups via the 5th pipe field in `config.env`. The display renders one group per page. Tap the top-right corner of the title bar (`GroupName >`) to advance to the next page.

### FreeRTOS Dual-Core Safety

Display runs on Core 1 (displayTask, priority 3). Scanner runs on Core 0 (scannerTask, priority 2). Cross-core display updates use a `volatile bool pending_refresh` flag — no direct TFT calls from Core 0, eliminating StoreProhibited/LoadProhibited crashes.

### Dynamic Footer (3 modes, tap to cycle)

- **System overview**: `alerts: 0  up: 7/7  uptime: 1:23h`
- **WiFi info**: `ssid: MyNetwork  -52dBm (good)`
- **Memory stats**: `ram: 15%  free: 218KB  heap: 195KB`

### Telegram Alerts

- Configurable failure threshold before alerting (default: 3)
- 5-minute cooldown between alerts per target
- Recovery notifications when targets come back up
- NTP-synchronized timestamps in all messages

## Hardware Requirements

- **Board**: ESP32-2432S028R (CYD variant)
- **TFT Display**: ST7789 (240x320), rotation 2
- **Touch Controller**: XPT2046 (resistive)
- **Storage**: microSD (VSPI) + SPIFFS (runtime)

### Pin Configuration

| Peripheral | Signal    | ESP32 Pin | Notes                      |
|------------|-----------|-----------|----------------------------|
| TFT (HSPI) | MOSI      | GPIO 13   |                            |
|            | MISO      | GPIO 12   |                            |
|            | SCLK      | GPIO 14   |                            |
|            | CS        | GPIO 15   | `TFT_CS`                   |
|            | DC        | GPIO 2    | `TFT_DC`                   |
|            | RST       | NC (-1)   | Not connected              |
|            | Backlight | GPIO 27   | Active HIGH                |
| Touch      | CS        | GPIO 33   | Shared HSPI bus            |
| (XPT2046)  | IRQ       | GPIO 36   |                            |
| SD Card    | MOSI      | GPIO 23   | VSPI                       |
|            | MISO      | GPIO 19   | VSPI                       |
|            | SCLK      | GPIO 18   | VSPI                       |
|            | CS        | GPIO 5    |                            |
| RGB LED    | R         | GPIO 16   | Configurable via config.env |
|            | G         | GPIO 17   |                            |
|            | B         | GPIO 20   |                            |

## Quick Start

1. **Clone**:
```bash
git clone https://github.com/Tech-Tweakers/nebula-monitor.git
cd nebula-monitor
```

2. **Configure** `data/config.env`:
```env
WIFI_SSID=YourWiFiName
WIFI_PASS=YourWiFiPassword
TELEGRAM_BOT_TOKEN=your_bot_token
TELEGRAM_CHAT_ID=your_chat_id

TARGET_1=Main Router|http://192.168.1.1/||PING|Local
TARGET_2=My API|https://api.example.com|/health|HEALTH_CHECK|Local
TARGET_3=GitHub|https://github.com||PING|Internet
```

3. **Upload**:
```bash
pio run --target uploadfs   # filesystem (config.env)
pio run --target upload     # firmware
pio device monitor          # serial output
```

## Configuration

All settings live in `data/config.env` on SPIFFS. The SD card sync feature copies an updated `config.env` from SD to SPIFFS on boot.

### Target Format

```
TARGET_N=NAME|URL|HEALTH_ENDPOINT|MONITOR_TYPE|GROUP
```

| Field | Description |
|-------|-------------|
| NAME | Display name (shown on screen) |
| URL | Full URL including protocol |
| HEALTH_ENDPOINT | Path for health check (empty for PING) |
| MONITOR_TYPE | `PING` or `HEALTH_CHECK` |
| GROUP | Page group name (e.g. `Local`, `Internet`, `Services`) |

Example:
```env
TARGET_1=Proxmox|http://192.168.1.128:8006/||PING|Local
TARGET_2=Router|http://192.168.1.1||PING|Local
TARGET_3=Polaris API|https://api.example.com|/health|HEALTH_CHECK|Services
TARGET_4=GitHub|https://github.com||PING|Internet
TARGET_5=Cloudflare|https://1.1.1.1||PING|Internet
```

### Key Settings

```env
# Performance
SCAN_INTERVAL_MS=30000
HTTP_TIMEOUT_MS=5000

# Alerts
MAX_FAILURES_BEFORE_ALERT=3
ALERT_COOLDOWN_MS=300000
ALERT_RECOVERY_COOLDOWN_MS=60000

# Touch calibration
TOUCH_X_MIN=200
TOUCH_X_MAX=3700
TOUCH_Y_MIN=240
TOUCH_Y_MAX=3800

# LED
LED_ACTIVE_HIGH=false
LED_BRIGHT_R=32
LED_BRIGHT_G=12
LED_BRIGHT_B=100

# Debug
ALL_LOGS_ENABLED=true
SILENT_MODE=false
```

## User Interface

### Main Screen

```
┌──────────────────────────────────────┐
│  NEBULA MONITOR              Local > │  ← Title (tap right to change page)
├──────────────────────────────────────┤
│ ▌ Main Router                  12ms  │
│ ▌ Polaris WEB                  85ms  │  ← Status items (tap to open modal)
│ ▌ Polaris API                  DOWN  │
├──────────────────────────────────────┤
│  alerts: 1  up: 2/3  uptime: 0:42h  │  ← Footer (tap to cycle modes)
└──────────────────────────────────────┘
```

### Status Colors

| Color | Meaning |
|-------|---------|
| Green | UP, latency < 500ms |
| Orange | UP, latency ≥ 500ms |
| Red | DOWN |
| Gray | UNKNOWN (first scan pending) |

### Target Detail Modal

Tap any target to open its modal:
- Status, latency, fail count
- Last down duration
- Time since last status change
- Tap `[X]` (top-right) to close

### LED Indicator

| Color | State |
|-------|-------|
| Blue | Scanning |
| Green | All targets up |
| Red solid | Targets down |
| Red blinking | WiFi disconnected |

## Development

### Build Commands

```bash
pio run                     # build
pio run --target upload     # upload firmware
pio run --target uploadfs   # upload SPIFFS (config.env)
pio device monitor          # serial monitor
```

### Debug Logging

```env
DEBUG_LOGS_ENABLED=true
TOUCH_LOGS_ENABLED=true
ALL_LOGS_ENABLED=true
```

Key prefixes: `[DISPLAY]`, `[NETWORK_MONITOR]`, `[HTTP]`, `[TELEGRAM]`, `[TOUCH]`, `[CONFIG]`

## Project Structure

```
nebula-monitor/
├── src/
│   ├── core/
│   │   ├── domain/
│   │   │   ├── target/          # Target model (status, latency, group, session stats)
│   │   │   ├── network_monitor/ # Scan orchestration
│   │   │   └── status/          # Status enum
│   │   └── infrastructure/
│   │       ├── http_client/     # PING + health check with SSL
│   │       ├── telegram_service/
│   │       ├── ntp_service/
│   │       ├── sdcard_manager/
│   │       ├── memory_manager/
│   │       └── logger/
│   ├── ui/
│   │   ├── display_manager/     # Raw TFT_eSPI UI, pagination, modal
│   │   ├── touch_handler/       # XPT2046 via SPI
│   │   └── led_controller/      # RGB LED PWM
│   ├── config/
│   │   └── config_loader/       # SPIFFS config.env parser
│   └── main.cpp
├── data/
│   └── config.env               # Runtime configuration (gitignored)
├── include/
│   └── User_Setup.h             # TFT_eSPI pin/driver config
└── platformio.ini
```

## Troubleshooting

**Config not loading**: Upload SPIFFS with `pio run --target uploadfs`. Check `config.env` has no spaces around `=`.

**Wrong colors on display**: All colors must use the `c()` helper (bitwise NOT). See [Color Inversion](#ui-architecture-raw-tft_espi).

**Touch not responding**: Check `TOUCH_CS_PIN=33` and `TOUCH_IRQ_PIN=36`. Recalibrate `TOUCH_X/Y_MIN/MAX` if needed.

**Targets not showing groups**: Ensure the 5th pipe field is present in all `TARGET_N` entries. Missing field defaults to `Default` group.

**Cross-core crash (StoreProhibited)**: Never call TFT functions directly from Core 0. Use `pending_refresh = true` and let displayTask handle the redraw.

**WiFi fails**: ESP32 only supports 2.4GHz. Check SSID/password and signal strength.

## Performance

After removing LVGL:

| Metric | Before (LVGL) | After (raw TFT_eSPI) |
|--------|---------------|----------------------|
| Flash  | 98%           | 84%                  |
| RAM    | 33%           | 15%                  |
| Free heap | ~50KB      | ~218KB               |

---

Made with care by [Tech-Tweakers](https://github.com/Tech-Tweakers) — Nebula Monitor v2.4
