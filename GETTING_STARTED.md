# Chess Clock - Setup & Next Steps

This document was created to preserve setup instructions when transitioning from the Aura workspace.

## Repo Structure

```
chess-clock/
├── chess-clock/              # Main Arduino sketch
│   ├── chess-clock.ino       # MVP touchscreen chess clock
│   └── config/               # Display configs for all 3 variants
├── TFT_eSPI/User_Setup.h     # TFT driver config (copy to library)
├── lvgl/src/lv_conf.h        # LVGL config (copy to library)
├── docs/                     # Web installer (GitHub Pages)
│   ├── index.html            # Installer page
│   └── manifests/            # 3 variant manifests
├── .github/workflows/        # CI/CD (disabled with `if: false`)
├── README.md                 # User docs
├── DEVELOPMENT.md            # Full dev plan & reference
└── LICENSE                   # MIT
```

## What's Included

- **Working MVP** with dual timers, touch control, pause/reset, settings dropdown
- **All 3 display variants** ported from Aura (2.4" ILI9341, 2.4" ST7789, 2.8" ILI9341)
- **Touch calibration** using tested CYD values from Aura debugging
- **Web installer** ready for GitHub Pages (just enable Pages on /docs)
- **GitHub Actions** disabled - remove `if: false` when ready to go live

## Next Steps

### 1. Initialize Git Repository
```powershell
cd c:\Users\Admin\Documents\GitHub\chess-clock
git init
git add .
git commit -m "Initial commit - MVP chess clock"
```

### 2. Create GitHub Repository
- Go to https://github.com/new
- Name: `chess-clock`
- Make it public or private as preferred
- Do NOT initialize with README (we already have one)

### 3. Push to GitHub
```powershell
git remote add origin https://github.com/OakesekAo/cyd-chess-clock.git
git branch -M main
git push -u origin main
```

### 4. Enable GitHub Pages (for web installer)
1. Go to repo Settings > Pages
2. Source: Deploy from a branch
3. Branch: `main`, Folder: `/docs`
4. Save
5. URL will be: `https://oakesekao.github.io/chess-clock/`

### 5. Test Locally with Arduino IDE
1. Open `chess-clock/chess-clock.ino` in Arduino IDE
2. Copy `TFT_eSPI/User_Setup.h` to your Arduino libraries TFT_eSPI folder
3. Copy `lvgl/src/lv_conf.h` to:
   - Your Arduino libraries lvgl folder
   - The parent libraries folder (next to lvgl folder)
4. Select your board variant by uncommenting in the .ino file:
   ```cpp
   // #define CHESS_CLOCK_24_ILI9341
   // #define CHESS_CLOCK_24_ST7789
   #define CHESS_CLOCK_28_ILI9341  // Default
   ```
5. Select board: ESP32 Dev Module
6. Upload!

### 6. When Ready for CI/CD
Edit `.github/workflows/build-firmware.yml`:
- Find the two `if: false` lines
- Remove them or change to `if: true`
- Push changes
- Create a release: `git tag v0.1.0 && git push --tags`

## Origin Reference

This project was created based on:
- **Aura Weather Display**: https://github.com/OakesekAo/Aura
- **Aura Installer**: https://github.com/OakesekAo/aura-installer
- Display/touch configuration was ported from extensive debugging in the Aura project

## Files to Review

- [README.md](README.md) - User-facing documentation
- [DEVELOPMENT.md](DEVELOPMENT.md) - Full development plan, troubleshooting, and technical details
