#pragma once
// Default to 2.8" ILI9341 unless a specific target is defined

#if defined(CHESS_CLOCK_24_ILI9341)
  #include "screen/cc_24_ili9341.h"
#elif defined(CHESS_CLOCK_24_ST7789)
  #include "screen/cc_24_st7789.h"
#elif defined(CHESS_CLOCK_28_ILI9341)
  #include "screen/cc_28_ili9341.h"
#else
  // Fallback / default
  #include "screen/cc_28_ili9341.h"
#endif
