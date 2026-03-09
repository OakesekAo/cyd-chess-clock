# ESP32 Chess Clock - Development Plan

## Overview

This project creates a touchscreen chess clock for ESP32 devices with ILI9341/ST7789 displays. The codebase and display/touch configuration is based on the [Aura Weather Display](https://github.com/OakesekAo/Aura) project.

## Project Origins

- **Parent Project**: [Aura](https://github.com/OakesekAo/Aura) - ESP32 Weather Display
- **Installer Reference**: [aura-installer](https://github.com/OakesekAo/aura-installer)
- **Display Work**: Extensive debugging was done for orientation, touch calibration, and display drivers

## Hardware Target

### Confirmed Working (from Aura project)
- ESP32-WROOM-32 (Cheap Yellow Display / CYD boards)
- 2.4" and 2.8" TFT displays
- XPT2046 touchscreen controller
- ILI9341 display driver (primary)
- ST7789 display driver (secondary)

### Pin Configuration
```
TFT Display (SPI):
  MISO: GPIO 12
  MOSI: GPIO 13
  SCLK: GPIO 14
  CS:   GPIO 15
  DC:   GPIO 2
  BL:   GPIO 21

Touch (XPT2046 - separate SPI):
  IRQ:  GPIO 36
  MOSI: GPIO 32
  MISO: GPIO 39
  CLK:  GPIO 25
  CS:   GPIO 33
```

### Touch Calibration
Using CYD standard calibration values:
- X range: 200 - 3700
- Y range: 240 - 3800

## MVP Features (v0.1.0)

### Implemented
- [x] Dual player countdown timers
- [x] Large, readable time display (48pt font)
- [x] Touch to switch turns
- [x] Player 1 time inverted (readable from opposite side)
- [x] Pause/Resume button
- [x] Reset button
- [x] Settings popup with time control dropdown
- [x] Time control presets (Bullet, Blitz, Rapid, Classical)
- [x] Persistent time control preference
- [x] Visual feedback (color changes for active/paused/flag)
- [x] Web installer page

### Not Yet Implemented
- [ ] Time increment/delay support (code exists, UI needs work)
- [ ] Move counter display
- [ ] Sound/haptic feedback
- [ ] Low time warning (flash/beep under 30s)
- [ ] Custom time control input
- [ ] Landscape orientation option
- [ ] Brightness control
- [ ] Flag animation

## Development Phases

### Phase 1: MVP (Current)
- Basic dual timer functionality
- Touch control
- Simple settings
- Web installer infrastructure

### Phase 2: Polish
- Sound effects (buzzer support)
- Low time warnings
- Move counter
- Better UI animations
- Increment working properly

### Phase 3: Features
- Custom time control editor
- Multiple game modes (Fischer, Bronstein delay)
- Game history/stats
- Brightness control
- Sleep mode

### Phase 4: Production
- Enable GitHub Actions
- Automated releases
- Proper versioning
- User documentation

## Building Locally

### Prerequisites
1. Arduino IDE 2.x or Arduino CLI
2. ESP32 Board Package v3.0.4
3. Libraries (install via Library Manager):
   - TFT_eSPI 2.5.43
   - lvgl 9.2.0
   - XPT2046_Touchscreen 1.4
   - ArduinoJson 7.0.4

### Setup Steps
1. Clone repository
2. Copy `TFT_eSPI/User_Setup.h` to Arduino libraries TFT_eSPI folder
3. Copy `lvgl/src/lv_conf.h` to:
   - Arduino libraries lvgl folder
   - Arduino libraries folder (parent of lvgl)
4. Open `chess-clock/chess-clock.ino`
5. Select board variant (uncomment in code)
6. Upload

### Variant Selection
In `chess-clock.ino`, uncomment ONE of these:
```cpp
// #define CHESS_CLOCK_24_ILI9341
// #define CHESS_CLOCK_24_ST7789
#define CHESS_CLOCK_28_ILI9341  // Default
```

## Web Installer Setup

### GitHub Pages
1. Push to GitHub
2. Go to Settings > Pages
3. Source: Deploy from a branch
4. Branch: main, folder: /docs
5. URL will be: `https://oakesekao.github.io/cyd-chess-clock/`

### Manifest Files
Located in `docs/manifests/`:
- `24_ILI9341.json`
- `24_ST7789.json`
- `28_ILI9341.json`

Manifests point to GitHub Releases for firmware binaries.

## Enabling CI/CD

When ready for automated builds:

1. Edit `.github/workflows/build-firmware.yml`
2. Remove `if: false` from `build:` and `release:` jobs
3. Push changes
4. Create a release tag: `git tag v0.1.0 && git push --tags`

## Troubleshooting

### Display Issues
- **White screen**: Check SPI wiring, verify User_Setup.h copied correctly
- **Wrong colors**: Try enabling `TFT_INVERSION_ON` in User_Setup.h
- **Upside down**: Modify rotation in code

### Touch Issues
- **No response**: Check touch SPI wiring (different pins from display)
- **Wrong position**: Adjust calibration values in touchscreen_read()
- **Enable debug**: Uncomment `#define CC_DEBUG_TOUCH` for serial output

### Build Errors
- **LVGL errors**: Ensure lv_conf.h is in correct locations
- **TFT errors**: Ensure User_Setup.h matches your display

## File Structure

```
chess-clock/
├── README.md                 # User documentation
├── DEVELOPMENT.md            # This file
├── LICENSE                   # MIT License
├── .gitignore
├── chess-clock/              # Arduino sketch folder
│   ├── chess-clock.ino       # Main application
│   └── config/
│       ├── screen_select.h   # Variant selector
│       └── screen/
│           ├── cc_24_ili9341.h
│           ├── cc_24_st7789.h
│           └── cc_28_ili9341.h
├── TFT_eSPI/
│   └── User_Setup.h          # Display driver config
├── lvgl/
│   └── src/
│       └── lv_conf.h         # LVGL config
├── docs/                     # GitHub Pages web installer
│   ├── index.html
│   ├── .nojekyll
│   └── manifests/
│       ├── 24_ILI9341.json
│       ├── 24_ST7789.json
│       └── 28_ILI9341.json
└── .github/
    └── workflows/
        └── build-firmware.yml  # CI/CD (disabled)
```

## References

- [LVGL Documentation](https://docs.lvgl.io/9.2/)
- [TFT_eSPI GitHub](https://github.com/Bodmer/TFT_eSPI)
- [ESP Web Tools](https://esphome.github.io/esp-web-tools/)
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32)
- [CYD Info](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display)
