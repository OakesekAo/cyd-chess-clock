/*
 * ESP32 Chess Clock
 * 
 * A touchscreen chess clock using LVGL for the UI.
 * Based on display/touch configuration from the Aura project.
 * 
 * Hardware: ESP32 + ILI9341/ST7789 TFT + XPT2046 Touch
 * 
 * Select your display variant by uncommenting ONE of these:
 */

// #define CHESS_CLOCK_24_ILI9341
// // #define CHESS_CLOCK_24_ST7789
// // #define CHESS_CLOCK_28_ILI9341  // Default - most common CYD variant
#define CHESS_CLOCK_24_ILI9341  // Testing 2.4" board

// Uncomment to run display diagnostic mode instead of chess clock
#define CC_DIAGNOSTIC_MODE

#include <Arduino.h>

// Only include heavy libraries when NOT in diagnostic mode
#ifndef CC_DIAGNOSTIC_MODE
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <Preferences.h>
#endif

#include <XPT2046_Touchscreen.h>

// Uncomment to enable touch debugging
// #define CC_DEBUG_TOUCH

#ifndef CC_DIAGNOSTIC_MODE
#include "config/screen_select.h"

// Touch controller pins (XPT2046 on separate SPI bus)
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33
#define LCD_BACKLIGHT_PIN 21

// Screen dimensions from config
#ifdef CC_TFT_WIDTH
  #define SCREEN_WIDTH CC_TFT_WIDTH
#else
  #define SCREEN_WIDTH 240
#endif

#ifdef CC_TFT_HEIGHT
  #define SCREEN_HEIGHT CC_TFT_HEIGHT
#else
  #define SCREEN_HEIGHT 320
#endif

#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))

// ============================================================================
// CHESS CLOCK STATE
// ============================================================================

enum GameState {
  STATE_IDLE,       // Waiting to start
  STATE_PLAYER1,    // Player 1's turn (timer running)
  STATE_PLAYER2,    // Player 2's turn (timer running)
  STATE_PAUSED,     // Game paused
  STATE_FINISHED    // One player's time ran out
};

// Time control presets (in seconds)
struct TimeControl {
  const char* name;
  uint32_t time_sec;
  uint32_t increment_sec;
};

static const TimeControl TIME_CONTROLS[] = {
  {"Bullet 1+0",    60,    0},
  {"Bullet 2+1",    120,   1},
  {"Blitz 3+0",     180,   0},
  {"Blitz 3+2",     180,   2},
  {"Blitz 5+0",     300,   0},
  {"Blitz 5+3",     300,   3},
  {"Rapid 10+0",    600,   0},
  {"Rapid 15+10",   900,   10},
  {"Classical 30+0", 1800, 0},
  {"Custom",        300,   0}
};
#define NUM_TIME_CONTROLS (sizeof(TIME_CONTROLS) / sizeof(TIME_CONTROLS[0]))

// Game state
static GameState game_state = STATE_IDLE;
static uint32_t player1_time_ms = 300000;  // 5 minutes default
static uint32_t player2_time_ms = 300000;
static uint32_t increment_ms = 0;
static uint32_t last_tick = 0;
static int current_time_control = 4;  // Default: Blitz 5+0
static int player1_moves = 0;
static int player2_moves = 0;

// Hardware
SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);
TFT_eSPI tft = TFT_eSPI();
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

// Preferences
static Preferences prefs;

// UI Components
static lv_obj_t* lbl_player1_time;
static lv_obj_t* lbl_player2_time;
static lv_obj_t* btn_player1;
static lv_obj_t* btn_player2;
static lv_obj_t* btn_pause;
static lv_obj_t* btn_reset;
static lv_obj_t* btn_settings;
static lv_obj_t* settings_win = nullptr;

// ============================================================================
// DISPLAY & TOUCH DRIVERS
// ============================================================================

void touchscreen_read(lv_indev_t* indev, lv_indev_data_t* data) {
  if (touchscreen.tirqTouched() && touchscreen.touched()) {
    TS_Point p = touchscreen.getPoint();

    // CYD (Cheap Yellow Display) calibration values
    int16_t min_x = 200, max_x = 3700;
    int16_t min_y = 240, max_y = 3800;
    
    int16_t raw_x = constrain(p.x, min_x, max_x);
    int16_t raw_y = constrain(p.y, min_y, max_y);
    
    int x = map(raw_x, min_x, max_x, 0, CC_TFT_WIDTH - 1);
    int y = map(raw_y, min_y, max_y, 0, CC_TFT_HEIGHT - 1);
    
    x = constrain(x, 0, CC_TFT_WIDTH - 1);
    y = constrain(y, 0, CC_TFT_HEIGHT - 1);

    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = x;
    data->point.y = y;
    
    #ifdef CC_DEBUG_TOUCH
    Serial.printf("Touch: raw(%d,%d) -> screen(%d,%d)\n", p.x, p.y, x, y);
    #endif
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void my_disp_flush(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t*)px_map, w * h, true);
  tft.endWrite();

  lv_display_flush_ready(disp);
}

// ============================================================================
// TIME FORMATTING
// ============================================================================

void format_time(uint32_t time_ms, char* buf, size_t buf_size) {
  uint32_t total_sec = time_ms / 1000;
  uint32_t minutes = total_sec / 60;
  uint32_t seconds = total_sec % 60;
  uint32_t tenths = (time_ms % 1000) / 100;
  
  if (minutes >= 10) {
    snprintf(buf, buf_size, "%02lu:%02lu", minutes, seconds);
  } else {
    snprintf(buf, buf_size, "%lu:%02lu.%lu", minutes, seconds, tenths);
  }
}

void update_time_display() {
  char buf[16];
  
  format_time(player1_time_ms, buf, sizeof(buf));
  lv_label_set_text(lbl_player1_time, buf);
  
  format_time(player2_time_ms, buf, sizeof(buf));
  lv_label_set_text(lbl_player2_time, buf);
}

// ============================================================================
// GAME LOGIC
// ============================================================================

void reset_game() {
  player1_time_ms = TIME_CONTROLS[current_time_control].time_sec * 1000;
  player2_time_ms = TIME_CONTROLS[current_time_control].time_sec * 1000;
  increment_ms = TIME_CONTROLS[current_time_control].increment_sec * 1000;
  player1_moves = 0;
  player2_moves = 0;
  game_state = STATE_IDLE;
  last_tick = millis();
  
  update_time_display();
  
  // Reset button colors
  lv_obj_set_style_bg_color(btn_player1, lv_color_hex(0x2196F3), 0);
  lv_obj_set_style_bg_color(btn_player2, lv_color_hex(0x2196F3), 0);
}

void switch_to_player1() {
  if (game_state == STATE_PLAYER2) {
    player2_time_ms += increment_ms;
    player2_moves++;
  }
  game_state = STATE_PLAYER1;
  last_tick = millis();
  
  lv_obj_set_style_bg_color(btn_player1, lv_color_hex(0x4CAF50), 0);  // Green - active
  lv_obj_set_style_bg_color(btn_player2, lv_color_hex(0x2196F3), 0);  // Blue - waiting
}

void switch_to_player2() {
  if (game_state == STATE_PLAYER1) {
    player1_time_ms += increment_ms;
    player1_moves++;
  }
  game_state = STATE_PLAYER2;
  last_tick = millis();
  
  lv_obj_set_style_bg_color(btn_player1, lv_color_hex(0x2196F3), 0);  // Blue - waiting
  lv_obj_set_style_bg_color(btn_player2, lv_color_hex(0x4CAF50), 0);  // Green - active
}

void toggle_pause() {
  if (game_state == STATE_PAUSED) {
    // Resume - restore previous state based on which timer was lower (approximation)
    if (player1_moves >= player2_moves && player1_moves > 0) {
      game_state = STATE_PLAYER2;
      lv_obj_set_style_bg_color(btn_player2, lv_color_hex(0x4CAF50), 0);
    } else if (player2_moves > 0) {
      game_state = STATE_PLAYER1;
      lv_obj_set_style_bg_color(btn_player1, lv_color_hex(0x4CAF50), 0);
    }
    last_tick = millis();
    lv_label_set_text(lv_obj_get_child(btn_pause, 0), LV_SYMBOL_PAUSE);
  } else if (game_state == STATE_PLAYER1 || game_state == STATE_PLAYER2) {
    game_state = STATE_PAUSED;
    lv_obj_set_style_bg_color(btn_player1, lv_color_hex(0xFF9800), 0);  // Orange - paused
    lv_obj_set_style_bg_color(btn_player2, lv_color_hex(0xFF9800), 0);
    lv_label_set_text(lv_obj_get_child(btn_pause, 0), LV_SYMBOL_PLAY);
  }
}

void update_timers() {
  if (game_state != STATE_PLAYER1 && game_state != STATE_PLAYER2) {
    return;
  }
  
  uint32_t now = millis();
  uint32_t elapsed = now - last_tick;
  last_tick = now;
  
  if (game_state == STATE_PLAYER1) {
    if (elapsed >= player1_time_ms) {
      player1_time_ms = 0;
      game_state = STATE_FINISHED;
      lv_obj_set_style_bg_color(btn_player1, lv_color_hex(0xF44336), 0);  // Red - lost
      Serial.println("Player 1 flagged!");
    } else {
      player1_time_ms -= elapsed;
    }
  } else if (game_state == STATE_PLAYER2) {
    if (elapsed >= player2_time_ms) {
      player2_time_ms = 0;
      game_state = STATE_FINISHED;
      lv_obj_set_style_bg_color(btn_player2, lv_color_hex(0xF44336), 0);  // Red - lost
      Serial.println("Player 2 flagged!");
    } else {
      player2_time_ms -= elapsed;
    }
  }
  
  update_time_display();
}

// ============================================================================
// UI EVENT HANDLERS
// ============================================================================

static void player1_btn_cb(lv_event_t* e) {
  if (game_state == STATE_IDLE) {
    switch_to_player2();  // Player 1 made first move, now player 2's turn
  } else if (game_state == STATE_PLAYER1) {
    switch_to_player2();
  }
}

static void player2_btn_cb(lv_event_t* e) {
  if (game_state == STATE_IDLE) {
    switch_to_player1();  // Player 2 starts, player 1's turn
  } else if (game_state == STATE_PLAYER2) {
    switch_to_player1();
  }
}

static void pause_btn_cb(lv_event_t* e) {
  toggle_pause();
}

static void reset_btn_cb(lv_event_t* e) {
  reset_game();
}

static void settings_btn_cb(lv_event_t* e);
static void close_settings_cb(lv_event_t* e);
static void time_control_changed_cb(lv_event_t* e);

// ============================================================================
// UI CREATION
// ============================================================================

void create_settings_window() {
  if (settings_win != nullptr) {
    lv_obj_delete(settings_win);
  }
  
  settings_win = lv_obj_create(lv_scr_act());
  lv_obj_set_size(settings_win, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 40);
  lv_obj_center(settings_win);
  lv_obj_set_style_bg_color(settings_win, lv_color_hex(0x1E1E1E), 0);
  lv_obj_set_style_border_color(settings_win, lv_color_hex(0x444444), 0);
  lv_obj_set_style_border_width(settings_win, 2, 0);
  lv_obj_set_style_radius(settings_win, 10, 0);
  
  // Title
  lv_obj_t* title = lv_label_create(settings_win);
  lv_label_set_text(title, "Time Control");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
  
  // Dropdown for time control
  lv_obj_t* dd = lv_dropdown_create(settings_win);
  
  // Build options string
  static char options[512];
  options[0] = '\0';
  for (int i = 0; i < NUM_TIME_CONTROLS; i++) {
    strcat(options, TIME_CONTROLS[i].name);
    if (i < NUM_TIME_CONTROLS - 1) {
      strcat(options, "\n");
    }
  }
  lv_dropdown_set_options(dd, options);
  lv_dropdown_set_selected(dd, current_time_control);
  lv_obj_set_width(dd, 180);
  lv_obj_align(dd, LV_ALIGN_TOP_MID, 0, 50);
  lv_obj_add_event_cb(dd, time_control_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);
  
  // Close button
  lv_obj_t* btn_close = lv_button_create(settings_win);
  lv_obj_set_size(btn_close, 100, 40);
  lv_obj_align(btn_close, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_obj_set_style_bg_color(btn_close, lv_color_hex(0x4CAF50), 0);
  lv_obj_add_event_cb(btn_close, close_settings_cb, LV_EVENT_CLICKED, NULL);
  
  lv_obj_t* lbl_close = lv_label_create(btn_close);
  lv_label_set_text(lbl_close, "Start Game");
  lv_obj_center(lbl_close);
}

static void settings_btn_cb(lv_event_t* e) {
  create_settings_window();
}

static void close_settings_cb(lv_event_t* e) {
  if (settings_win != nullptr) {
    lv_obj_delete(settings_win);
    settings_win = nullptr;
  }
  reset_game();
}

static void time_control_changed_cb(lv_event_t* e) {
  lv_obj_t* dd = (lv_obj_t*)lv_event_get_target(e);
  current_time_control = lv_dropdown_get_selected(dd);
  
  // Save preference
  prefs.putInt("timeControl", current_time_control);
}

void create_ui() {
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x121212), 0);
  
  // Calculate layout - split screen in half horizontally for the two players
  int timer_height = (SCREEN_HEIGHT - 60) / 2;  // Leave space for controls
  
  // Player 1 button (top half) - rotated 180 degrees for opponent viewing
  btn_player1 = lv_button_create(lv_scr_act());
  lv_obj_set_size(btn_player1, SCREEN_WIDTH - 10, timer_height - 5);
  lv_obj_align(btn_player1, LV_ALIGN_TOP_MID, 0, 5);
  lv_obj_set_style_bg_color(btn_player1, lv_color_hex(0x2196F3), 0);
  lv_obj_set_style_radius(btn_player1, 10, 0);
  lv_obj_add_event_cb(btn_player1, player1_btn_cb, LV_EVENT_CLICKED, NULL);
  
  lbl_player1_time = lv_label_create(btn_player1);
  lv_label_set_text(lbl_player1_time, "5:00.0");
  lv_obj_set_style_text_font(lbl_player1_time, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(lbl_player1_time, lv_color_hex(0xFFFFFF), 0);
  lv_obj_center(lbl_player1_time);
  // Rotate player 1's time 180 degrees so opponent can read it
  lv_obj_set_style_transform_rotation(lbl_player1_time, 1800, 0);
  lv_obj_set_style_transform_pivot_x(lbl_player1_time, lv_pct(50), 0);
  lv_obj_set_style_transform_pivot_y(lbl_player1_time, lv_pct(50), 0);
  
  // Player 2 button (bottom half)
  btn_player2 = lv_button_create(lv_scr_act());
  lv_obj_set_size(btn_player2, SCREEN_WIDTH - 10, timer_height - 5);
  lv_obj_align(btn_player2, LV_ALIGN_BOTTOM_MID, 0, -55);
  lv_obj_set_style_bg_color(btn_player2, lv_color_hex(0x2196F3), 0);
  lv_obj_set_style_radius(btn_player2, 10, 0);
  lv_obj_add_event_cb(btn_player2, player2_btn_cb, LV_EVENT_CLICKED, NULL);
  
  lbl_player2_time = lv_label_create(btn_player2);
  lv_label_set_text(lbl_player2_time, "5:00.0");
  lv_obj_set_style_text_font(lbl_player2_time, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(lbl_player2_time, lv_color_hex(0xFFFFFF), 0);
  lv_obj_center(lbl_player2_time);
  
  // Control buttons row at bottom
  int btn_width = (SCREEN_WIDTH - 30) / 3;
  int btn_y = SCREEN_HEIGHT - 45;
  
  // Settings button
  btn_settings = lv_button_create(lv_scr_act());
  lv_obj_set_size(btn_settings, btn_width, 40);
  lv_obj_set_pos(btn_settings, 5, btn_y);
  lv_obj_set_style_bg_color(btn_settings, lv_color_hex(0x607D8B), 0);
  lv_obj_add_event_cb(btn_settings, settings_btn_cb, LV_EVENT_CLICKED, NULL);
  
  lv_obj_t* lbl_settings = lv_label_create(btn_settings);
  lv_label_set_text(lbl_settings, LV_SYMBOL_SETTINGS);
  lv_obj_center(lbl_settings);
  
  // Pause button
  btn_pause = lv_button_create(lv_scr_act());
  lv_obj_set_size(btn_pause, btn_width, 40);
  lv_obj_set_pos(btn_pause, 10 + btn_width, btn_y);
  lv_obj_set_style_bg_color(btn_pause, lv_color_hex(0xFF9800), 0);
  lv_obj_add_event_cb(btn_pause, pause_btn_cb, LV_EVENT_CLICKED, NULL);
  
  lv_obj_t* lbl_pause = lv_label_create(btn_pause);
  lv_label_set_text(lbl_pause, LV_SYMBOL_PAUSE);
  lv_obj_center(lbl_pause);
  
  // Reset button
  btn_reset = lv_button_create(lv_scr_act());
  lv_obj_set_size(btn_reset, btn_width, 40);
  lv_obj_set_pos(btn_reset, 15 + btn_width * 2, btn_y);
  lv_obj_set_style_bg_color(btn_reset, lv_color_hex(0xF44336), 0);
  lv_obj_add_event_cb(btn_reset, reset_btn_cb, LV_EVENT_CLICKED, NULL);
  
  lv_obj_t* lbl_reset = lv_label_create(btn_reset);
  lv_label_set_text(lbl_reset, LV_SYMBOL_REFRESH);
  lv_obj_center(lbl_reset);
}

#endif  // !CC_DIAGNOSTIC_MODE (end of chess clock code)

// ============================================================================
// DIAGNOSTIC MODE v7 - 2.4" ILI9341 + TOUCH TEST
// ============================================================================

#ifdef CC_DIAGNOSTIC_MODE

#include <SPI.h>

// RGB LED pins - your board variant has G/B swapped
#define RGB_LED_R 4
#define RGB_LED_G 17
#define RGB_LED_B 16

// DISPLAY PINS - confirmed from pin scanner + Aura 2.4" config
#define HSPI_MISO 12
#define HSPI_MOSI 13
#define HSPI_SCLK 14
#define TFT_CS_PIN 15
#define TFT_DC_PIN 27   // KEY FIX: 2.4" uses DC=27, not DC=2!
#define TFT_BL_PIN 21

// TOUCH PINS - from Aura (XPT2046 on separate VSPI bus)
#define TOUCH_CLK  25
#define TOUCH_MISO 39
#define TOUCH_MOSI 32
#define TOUCH_CS   33
#define TOUCH_IRQ  36

// Touch calibration for 2.4" ILI9341 (from Aura)
#define TOUCH_MIN_X 200
#define TOUCH_MAX_X 3700
#define TOUCH_MIN_Y 240
#define TOUCH_MAX_Y 3800

SPIClass hspi(HSPI);
SPIClass touchSPI(VSPI);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

void writeCmd(uint8_t cmd) {
  digitalWrite(TFT_DC_PIN, LOW);
  digitalWrite(TFT_CS_PIN, LOW);
  hspi.transfer(cmd);
  digitalWrite(TFT_CS_PIN, HIGH);
}

void writeData8(uint8_t data) {
  digitalWrite(TFT_DC_PIN, HIGH);
  digitalWrite(TFT_CS_PIN, LOW);
  hspi.transfer(data);
  digitalWrite(TFT_CS_PIN, HIGH);
}

void writeDataBuf(const uint8_t* data, size_t len) {
  digitalWrite(TFT_DC_PIN, HIGH);
  digitalWrite(TFT_CS_PIN, LOW);
  for (size_t i = 0; i < len; i++) {
    hspi.transfer(data[i]);
  }
  digitalWrite(TFT_CS_PIN, HIGH);
}

// Full ILI9341 initialization
void initDisplay() {
  Serial.println("  [Reset]");
  writeCmd(0x01);  // Software reset
  delay(150);
  
  Serial.println("  [Power Control]");
  writeCmd(0xCB); uint8_t d1[] = {0x39, 0x2C, 0x00, 0x34, 0x02}; writeDataBuf(d1, 5);
  writeCmd(0xCF); uint8_t d2[] = {0x00, 0xC1, 0x30}; writeDataBuf(d2, 3);
  writeCmd(0xE8); uint8_t d3[] = {0x85, 0x00, 0x78}; writeDataBuf(d3, 3);
  writeCmd(0xEA); uint8_t d4[] = {0x00, 0x00}; writeDataBuf(d4, 2);
  writeCmd(0xED); uint8_t d5[] = {0x64, 0x03, 0x12, 0x81}; writeDataBuf(d5, 4);
  writeCmd(0xF7); writeData8(0x20);
  writeCmd(0xC0); writeData8(0x23);
  writeCmd(0xC1); writeData8(0x10);
  writeCmd(0xC5); uint8_t d6[] = {0x3E, 0x28}; writeDataBuf(d6, 2);
  writeCmd(0xC7); writeData8(0x86);
  
  Serial.println("  [Memory/Pixel]");
  writeCmd(0x36); writeData8(0x48);  // Memory Access Control: MX, BGR
  writeCmd(0x3A); writeData8(0x55);  // 16-bit color
  
  Serial.println("  [Timing]");
  writeCmd(0xB1); uint8_t d7[] = {0x00, 0x18}; writeDataBuf(d7, 2);
  writeCmd(0xB6); uint8_t d8[] = {0x08, 0x82, 0x27}; writeDataBuf(d8, 3);
  writeCmd(0xF2); writeData8(0x00);
  
  Serial.println("  [Gamma]");
  writeCmd(0x26); writeData8(0x01);
  writeCmd(0xE0); uint8_t gp[] = {0x0F,0x31,0x2B,0x0C,0x0E,0x08,0x4E,0xF1,0x37,0x07,0x10,0x03,0x0E,0x09,0x00}; writeDataBuf(gp, 15);
  writeCmd(0xE1); uint8_t gn[] = {0x00,0x0E,0x14,0x03,0x11,0x07,0x31,0xC1,0x48,0x08,0x0F,0x0C,0x31,0x36,0x0F}; writeDataBuf(gn, 15);
  
  Serial.println("  [Display ON]");
  writeCmd(0x11); delay(120);  // Sleep out
  writeCmd(0x29); delay(50);   // Display on
}

void fillScreen(uint16_t color) {
  // Set full screen window (240x320)
  writeCmd(0x2A);  // Column address
  uint8_t col[] = {0x00, 0x00, 0x00, 0xEF};  // 0-239
  writeDataBuf(col, 4);
  
  writeCmd(0x2B);  // Row address
  uint8_t row[] = {0x00, 0x00, 0x01, 0x3F};  // 0-319
  writeDataBuf(row, 4);
  
  writeCmd(0x2C);  // Memory write
  digitalWrite(TFT_DC_PIN, HIGH);
  digitalWrite(TFT_CS_PIN, LOW);
  
  uint16_t swapped = (color >> 8) | (color << 8);
  for (uint32_t i = 0; i < 240UL * 320UL; i++) {
    hspi.transfer16(swapped);
  }
  digitalWrite(TFT_CS_PIN, HIGH);
}

// Draw a small crosshair at position
void drawCrosshair(int x, int y, uint16_t color) {
  // Clamp to screen
  if (x < 5) x = 5;
  if (x > 234) x = 234;
  if (y < 5) y = 5;
  if (y > 314) y = 314;
  
  uint16_t swapped = (color >> 8) | (color << 8);
  
  // Horizontal line (10px centered at x,y)
  writeCmd(0x2A);
  uint8_t col1[] = {0, (uint8_t)(x-5), 0, (uint8_t)(x+5)};
  writeDataBuf(col1, 4);
  writeCmd(0x2B);
  uint8_t row1[] = {(uint8_t)(y >> 8), (uint8_t)(y & 0xFF), (uint8_t)(y >> 8), (uint8_t)(y & 0xFF)};
  writeDataBuf(row1, 4);
  writeCmd(0x2C);
  digitalWrite(TFT_DC_PIN, HIGH);
  digitalWrite(TFT_CS_PIN, LOW);
  for (int i = 0; i < 11; i++) hspi.transfer16(swapped);
  digitalWrite(TFT_CS_PIN, HIGH);
  
  // Vertical line (10px centered at x,y)
  writeCmd(0x2A);
  uint8_t col2[] = {0, (uint8_t)x, 0, (uint8_t)x};
  writeDataBuf(col2, 4);
  writeCmd(0x2B);
  uint8_t row2[] = {(uint8_t)((y-5) >> 8), (uint8_t)((y-5) & 0xFF), (uint8_t)((y+5) >> 8), (uint8_t)((y+5) & 0xFF)};
  writeDataBuf(row2, 4);
  writeCmd(0x2C);
  digitalWrite(TFT_DC_PIN, HIGH);
  digitalWrite(TFT_CS_PIN, LOW);
  for (int i = 0; i < 11; i++) hspi.transfer16(swapped);
  digitalWrite(TFT_CS_PIN, HIGH);
}

#define FIRMWARE_VERSION "v7.1-touch"
#define BUILD_ID "2026-03-12-B"

void diagnostic_setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial.println("\n\n");
  Serial.println("################################################");
  Serial.println("#                                              #");
  Serial.println("#   ESP32 CHESS CLOCK DIAGNOSTIC " FIRMWARE_VERSION "    #");
  Serial.println("#   Build: " BUILD_ID "                        #");
  Serial.println("#                                              #");
  Serial.println("################################################");
  Serial.println("");
  Serial.println(">>> If you see PIN SCANNER or 42 tests, you have OLD firmware!");
  Serial.println(">>> This version does: display test + touch test only");
  Serial.println("");
  Serial.printf("Chip: %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.println("");
  Serial.println("DISPLAY PINS (DC=27 confirmed working):");
  Serial.printf("  SPI:  SCLK=%d MOSI=%d MISO=%d CS=%d\n", HSPI_SCLK, HSPI_MOSI, HSPI_MISO, TFT_CS_PIN);
  Serial.printf("  TFT:  DC=%d  BL=%d\n", TFT_DC_PIN, TFT_BL_PIN);
  Serial.println("TOUCH PINS (XPT2046 on VSPI):");
  Serial.printf("  SPI:  CLK=%d MOSI=%d MISO=%d CS=%d IRQ=%d\n", TOUCH_CLK, TOUCH_MOSI, TOUCH_MISO, TOUCH_CS, TOUCH_IRQ);
  Serial.println("TOUCH CALIBRATION (2.4\" ILI9341 from Aura):");
  Serial.printf("  X: %d - %d\n", TOUCH_MIN_X, TOUCH_MAX_X);
  Serial.printf("  Y: %d - %d\n", TOUCH_MIN_Y, TOUCH_MAX_Y);
  Serial.println("================================================\n");
  
  // LED init
  pinMode(RGB_LED_R, OUTPUT);
  pinMode(RGB_LED_G, OUTPUT);
  pinMode(RGB_LED_B, OUTPUT);
  digitalWrite(RGB_LED_R, HIGH);
  digitalWrite(RGB_LED_G, HIGH);  
  digitalWrite(RGB_LED_B, HIGH);
  
  // Backlight ON
  Serial.println("[1] Backlight ON...");
  pinMode(TFT_BL_PIN, OUTPUT);
  digitalWrite(TFT_BL_PIN, HIGH);
  
  // Display SPI init
  Serial.println("[2] Display SPI init (HSPI)...");
  pinMode(TFT_CS_PIN, OUTPUT);
  pinMode(TFT_DC_PIN, OUTPUT);
  digitalWrite(TFT_CS_PIN, HIGH);
  digitalWrite(TFT_DC_PIN, HIGH);
  hspi.begin(HSPI_SCLK, HSPI_MISO, HSPI_MOSI, TFT_CS_PIN);
  hspi.setFrequency(40000000);
  hspi.setDataMode(SPI_MODE0);
  Serial.println("    HSPI @ 40MHz - OK");
  
  // Display init
  Serial.println("[3] Display init...");
  initDisplay();
  Serial.println("    ILI9341 initialized");
  
  // Color test
  Serial.println("[4] Display color test...");
  
  Serial.println("    RED");
  fillScreen(0xF800);
  digitalWrite(RGB_LED_R, LOW);
  delay(1500);
  digitalWrite(RGB_LED_R, HIGH);
  
  Serial.println("    GREEN");
  fillScreen(0x07E0);
  digitalWrite(RGB_LED_G, LOW);
  delay(1500);
  digitalWrite(RGB_LED_G, HIGH);
  
  Serial.println("    BLUE");
  fillScreen(0x001F);
  digitalWrite(RGB_LED_B, LOW);
  delay(1500);
  digitalWrite(RGB_LED_B, HIGH);
  
  // Touch init
  Serial.println("\n[5] Touch SPI init (VSPI)...");
  touchSPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  ts.begin(touchSPI);
  ts.setRotation(0);  // Match display rotation
  Serial.println("    XPT2046 initialized on VSPI");
  
  // Prepare for touch test
  Serial.println("\n[6] Touch test mode starting...");
  fillScreen(0x0000);  // Black background
  
  Serial.println("\n========================================");
  Serial.println("DISPLAY TEST COMPLETE");
  Serial.println("Now entering TOUCH TEST mode:");
  Serial.println("");
  Serial.println("  Touch the screen - watch for:");
  Serial.println("  1. Crosshair appears at touch point");
  Serial.println("  2. Serial output shows coordinates");
  Serial.println("");
  Serial.println("  Good calibration = crosshair under finger");
  Serial.println("  Bad calibration = crosshair offset");
  Serial.println("========================================\n");
}

void diagnostic_loop() {
  static uint32_t lastTouch = 0;
  static int lastX = -1, lastY = -1;
  
  // Check for touch
  if (ts.tirqTouched() && ts.touched()) {
    TS_Point p = ts.getPoint();
    
    // Raw values
    int raw_x = p.x;
    int raw_y = p.y;
    int raw_z = p.z;
    
    // Apply calibration (from Aura 2.4" ILI9341)
    int constrained_x = constrain(raw_x, TOUCH_MIN_X, TOUCH_MAX_X);
    int constrained_y = constrain(raw_y, TOUCH_MIN_Y, TOUCH_MAX_Y);
    
    // Map to screen coordinates
    int screen_x = map(constrained_x, TOUCH_MIN_X, TOUCH_MAX_X, 0, 239);
    int screen_y = map(constrained_y, TOUCH_MIN_Y, TOUCH_MAX_Y, 0, 319);
    
    // Final bounds check
    screen_x = constrain(screen_x, 0, 239);
    screen_y = constrain(screen_y, 0, 319);
    
    // Only process if enough time passed (debounce)
    if (millis() - lastTouch > 50) {
      lastTouch = millis();
      
      // Erase previous crosshair
      if (lastX >= 0 && lastY >= 0) {
        drawCrosshair(lastX, lastY, 0x0000);  // Black
      }
      
      // Draw new crosshair
      drawCrosshair(screen_x, screen_y, 0xFFFF);  // White
      lastX = screen_x;
      lastY = screen_y;
      
      // LED feedback
      digitalWrite(RGB_LED_G, LOW);
      
      // Debug output
      Serial.printf("TOUCH: raw(%d,%d,z=%d) -> screen(%d,%d)\n", 
                    raw_x, raw_y, raw_z, screen_x, screen_y);
    }
  } else {
    // No touch - turn off LED
    digitalWrite(RGB_LED_G, HIGH);
  }
  
  delay(10);
}

void setup() {
  diagnostic_setup();
}

void loop() {
  diagnostic_loop();
}

#else  // Normal chess clock mode

// ============================================================================
// SETUP & LOOP
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("=== ESP32 Chess Clock ===");
  Serial.printf("Chip Model: %s\n", ESP.getChipModel());
  Serial.printf("CPU Freq: %d MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("Screen: %dx%d\n", SCREEN_WIDTH, SCREEN_HEIGHT);
  Serial.println("========================");
  
  // Initialize TFT
  tft.init();
  pinMode(LCD_BACKLIGHT_PIN, OUTPUT);
  analogWrite(LCD_BACKLIGHT_PIN, 255);  // Full brightness
  
  // Initialize LVGL
  lv_init();
  
  // Initialize touchscreen
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(0);
  
  // Create LVGL display
  lv_display_t* disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_display_set_flush_cb(disp, my_disp_flush);
  lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
  
  // Create touch input device
  lv_indev_t* indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, touchscreen_read);
  
  // Load preferences
  prefs.begin("chessclock", false);
  current_time_control = prefs.getInt("timeControl", 4);  // Default: Blitz 5+0
  
  // Create UI
  create_ui();
  reset_game();
  
  Serial.println("Chess Clock ready!");
}

void loop() {
  lv_timer_handler();
  update_timers();
  
  lv_tick_inc(5);
  delay(5);
}
#endif  // CC_DIAGNOSTIC_MODE
