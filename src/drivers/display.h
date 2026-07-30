// display.h — panel bring-up (esp_lcd, ST7789-class over SPI) plus LVGL 9
// glue. This file is the only place that knows which controller is fitted;
// swapping to a GC9A01/ST7796 panel means touching only display.cpp.
//
// Buffering: two partial buffers (1/8 screen each) in internal DMA-capable
// RAM, flushed via esp_lcd DMA with the trans-done callback releasing LVGL.
// PSRAM holds LVGL heap/objects, not the flush buffers — SPI DMA from PSRAM
// has restrictions on the S3 and partial dual buffers are plenty at this
// resolution. See docs/decisions.md.

#pragma once

// Panel geometry — landscape. CALIBRATE to the actual 4" panel fitted
// (ST7789 = 320x240 max; a 480x320 panel means ST7796 init instead).
#define DISPLAY_H_RES 320
#define DISPLAY_V_RES 240

namespace display {

// Initializes SPI bus, panel, backlight, LVGL core, and registers the LVGL
// display driver. Call once from setup() before any UI code.
void init();

} // namespace display
