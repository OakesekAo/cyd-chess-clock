// Chess Clock 2.4" ILI9341 screen config
// Updated with pins confirmed working from pin scanner + Aura calibration
#pragma once

#define CC_TFT_DRIVER ILI9341_2_DRIVER
#define CC_TFT_WIDTH  240
#define CC_TFT_HEIGHT 320
#define CC_TFT_BL   21
#define CC_TFT_BACKLIGHT_ON HIGH

// Display SPI (HSPI)
#define CC_TFT_MISO 12
#define CC_TFT_MOSI 13
#define CC_TFT_SCLK 14
#define CC_TFT_CS   15
#define CC_TFT_DC   27    // KEY: 2.4" uses DC=27 (not DC=2 like 2.8")
#define CC_TFT_RST  -1

// Touch SPI (VSPI) - XPT2046
#define CC_TOUCH_CLK  25
#define CC_TOUCH_MISO 39
#define CC_TOUCH_MOSI 32
#define CC_TOUCH_CS   33
#define CC_TOUCH_IRQ  36

// Touch calibration for 2.4" ILI9341 (from Aura)
#define CC_TOUCH_MIN_X 200
#define CC_TOUCH_MAX_X 3700
#define CC_TOUCH_MIN_Y 240
#define CC_TOUCH_MAX_Y 3800

#define CC_SPI_FREQUENCY 40000000
#define CC_SPI_TOUCH_FREQUENCY 2500000
#define CC_USE_HSPI_PORT 1
