// Chess Clock 2.4" ST7789 screen config
#pragma once

#define CC_TFT_DRIVER ST7789_DRIVER
#define CC_TFT_WIDTH  240
#define CC_TFT_HEIGHT 320
#define CC_TFT_BL   21
#define CC_TFT_BACKLIGHT_ON HIGH
#define CC_TFT_MISO 12
#define CC_TFT_MOSI 13
#define CC_TFT_SCLK 14
#define CC_TFT_CS   15
#define CC_TFT_DC    2
#define CC_TFT_RST  -1
#define CC_TOUCH_CS 33
#define CC_SPI_FREQUENCY 55000000
#define CC_SPI_TOUCH_FREQUENCY 2500000
#define CC_USE_HSPI_PORT 1
