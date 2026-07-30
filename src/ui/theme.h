// theme.h — dark, high-contrast palette. Sunlight-readable, glanceable,
// no decorative clutter. Colors centralized so the whole UI shifts together.

#pragma once

#include <lvgl.h>

namespace theme {

static inline lv_color_t bg()         { return lv_color_hex(0x000000); }
static inline lv_color_t cell_bg()    { return lv_color_hex(0x101418); }
static inline lv_color_t text()       { return lv_color_hex(0xF2F2F2); }
static inline lv_color_t text_dim()   { return lv_color_hex(0x8A9299); }
static inline lv_color_t bar_track()  { return lv_color_hex(0x22282E); }
static inline lv_color_t ok()         { return lv_color_hex(0x35C86E); }
static inline lv_color_t warn()       { return lv_color_hex(0xFFB020); }
static inline lv_color_t crit()       { return lv_color_hex(0xE5304C); }
static inline lv_color_t stale()      { return lv_color_hex(0x555C63); }

} // namespace theme
