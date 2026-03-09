# ESP32 Chess Clock

A touchscreen chess clock for ESP32 with LVGL UI, supporting multiple display variants.

## 🎯 Project Status: MVP Development

This is a barebones MVP touchscreen chess clock based on the [Aura](https://github.com/OakesekAo/Aura) project's display/touch configuration.

## Supported Display Variants

| Variant | Display | Driver | Status |
|---------|---------|--------|--------|
| 2.4" ILI9341 | 240x320 | ILI9341 | 🔧 Testing |
| 2.4" ST7789 | 240x320 | ST7789 | 🔧 Testing |
| 2.8" ILI9341 | 240x320 | ILI9341 | 🔧 Testing |

## Hardware Requirements

- ESP32 DevKit (ESP-WROOM-32)
- 2.4" or 2.8" TFT LCD with ILI9341 or ST7789 driver
- XPT2046 Touch Controller
- USB cable for programming/power

### Pin Configuration (CYD - Cheap Yellow Display)

| Function | GPIO |
|----------|------|
| TFT_MISO | 12 |
| TFT_MOSI | 13 |
| TFT_SCLK | 14 |
| TFT_CS | 15 |
| TFT_DC | 2 |
| TFT_BL | 21 |
| TOUCH_CS | 33 |
| TOUCH_IRQ | 36 |
| TOUCH_MOSI | 32 |
| TOUCH_MISO | 39 |
| TOUCH_CLK | 25 |

## Features (MVP)

- [x] Dual player timers with large display
- [x] Touch to switch turns
- [x] Pause/Resume functionality
- [x] Settings screen (time control selection)
- [x] Multiple time control presets (Bullet, Blitz, Rapid, Classical)
- [ ] Increment/Delay support
- [ ] Move counter
- [ ] Sound/haptic feedback
- [ ] Low time warning

## Web Installer

Flash directly from your browser (Chrome/Edge required):
**[Install Chess Clock](https://oakesekao.github.io/cyd-chess-clock/)**

## Building Locally

### Prerequisites

1. Arduino IDE or Arduino CLI
2. ESP32 Board Package (v3.0.4+)
3. Required Libraries:
   - TFT_eSPI 2.5.43
   - lvgl 9.2.0
   - XPT2046_Touchscreen 1.4
   - ArduinoJson 7.0.4

### Setup

1. Clone this repository
2. Copy `TFT_eSPI/User_Setup.h` to your TFT_eSPI library folder
3. Copy `lvgl/src/lv_conf.h` to your lvgl library folder (and parent directory)
4. Open `chess-clock/chess-clock.ino` in Arduino IDE
5. Select your board variant by uncommenting the appropriate define:
   ```cpp
   // #define CHESS_CLOCK_24_ILI9341
   // #define CHESS_CLOCK_24_ST7789
   #define CHESS_CLOCK_28_ILI9341  // Default
   ```
6. Select board: ESP32 Dev Module
7. Upload!

## License

MIT License - see [LICENSE](LICENSE) file