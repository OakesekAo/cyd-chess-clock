/* 
 * TFT_eSPI User_Setup.h for ESP32 Chess Clock
 * Based on CYD (Cheap Yellow Display) configuration
 * 
 * Copy this file to your TFT_eSPI library folder
 */

#define USER_SETUP_INFO "ESP32_Chess_Clock"

// ##################################################################################
// Section 1. Driver selection
// ##################################################################################

// Use ILI9341_2_DRIVER for better compatibility with CYD displays
#define ILI9341_2_DRIVER

// For ST7789 variant, comment out ILI9341_2_DRIVER and uncomment:
// #define ST7789_DRIVER

#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// ##################################################################################
// Section 2. Pin definitions for ESP32
// ##################################################################################

#define TFT_BL   21            // LED back-light control pin
#define TFT_BACKLIGHT_ON HIGH  // Level to turn ON back-light

// ESP32 SPI pins (CYD standard configuration)
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15  // Chip select control pin
#define TFT_DC    2  // Data Command control pin
#define TFT_RST  -1  // Set to -1 if connected to ESP32 RST

// Touch controller chip select (XPT2046)
#define TOUCH_CS 33

// ##################################################################################
// Section 3. Fonts
// ##################################################################################

#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font
#define LOAD_FONT2  // Font 2. Small 16 pixel high font
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font
#define LOAD_FONT6  // Font 6. Large 48 pixel font (numbers and :-.apm)
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font (numbers and :-.)
#define LOAD_FONT8  // Font 8. Large 75 pixel font (numbers and :-.)
#define LOAD_GFXFF  // FreeFonts

#define SMOOTH_FONT

// ##################################################################################
// Section 4. SPI settings
// ##################################################################################

// Reduced SPI frequency for better compatibility (some CYD boards need slower speeds)
#define SPI_FREQUENCY  27000000
#define SPI_READ_FREQUENCY  16000000
#define SPI_TOUCH_FREQUENCY  2500000

// Use HSPI port (CYD boards use HSPI for display)
#define USE_HSPI_PORT
