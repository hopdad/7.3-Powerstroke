/* lv_conf.h — LVGL 9.x configuration for OBS Gauge.
 * Picked up via -D LV_CONF_INCLUDE_SIMPLE -I include in platformio.ini. */

#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH 16

/* millis()-driven tick is registered in display.cpp via lv_tick_set_cb() */

/* Memory: LVGL heap in PSRAM would need a custom alloc; the default 64K
 * internal pool is fine for this UI (8 static cells + one overlay). */
#define LV_MEM_SIZE (64 * 1024U)

#define LV_DEF_REFR_PERIOD 33   /* ms — ~30 FPS cap, plenty for gauges */

/* Only the widgets this UI uses */
#define LV_USE_LABEL   1
#define LV_USE_BAR     1
#define LV_USE_ARC     1

/* Fonts: small caption, mid unit text, large numeric readout */
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_36 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/* No filesystem, no images from storage, keep the build lean */
#define LV_USE_FS_STDIO 0
#define LV_USE_LOG 0

#endif /* LV_CONF_H */
