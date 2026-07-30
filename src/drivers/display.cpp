#include "display.h"

#include <Arduino.h>
#include <lvgl.h>

#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_heap_caps.h"

#include "pins.h"

namespace display {

static esp_lcd_panel_handle_t s_panel = nullptr;
static lv_display_t* s_lvgl_disp = nullptr;

// Two partial buffers, 1/8 of the screen each, internal DMA RAM.
static const size_t BUF_PIXELS = DISPLAY_H_RES * (DISPLAY_V_RES / 8);

// esp_lcd finished the DMA transfer → let LVGL reuse the buffer.
static bool on_color_trans_done(esp_lcd_panel_io_handle_t,
                                esp_lcd_panel_io_event_data_t*, void* user_ctx) {
    lv_display_flush_ready((lv_display_t*)user_ctx);
    return false;
}

static void flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    // ST7789 wants RGB565 big-endian; LVGL renders little-endian.
    lv_draw_sw_rgb565_swap(px_map,
                           (uint32_t)(area->x2 - area->x1 + 1) *
                           (uint32_t)(area->y2 - area->y1 + 1));
    esp_lcd_panel_draw_bitmap(s_panel, area->x1, area->y1,
                              area->x2 + 1, area->y2 + 1, px_map);
    // flush_ready fires from on_color_trans_done
    (void)disp;
}

void init() {
    // --- SPI bus (shared with SD later; SD gets its own CS) ---
    spi_bus_config_t buscfg = {};
    buscfg.sclk_io_num = PIN_SPI2_SCLK;
    buscfg.mosi_io_num = PIN_SPI2_MOSI;
    buscfg.miso_io_num = PIN_SPI2_MISO;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = BUF_PIXELS * sizeof(uint16_t);
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // --- Panel IO ---
    esp_lcd_panel_io_handle_t io = nullptr;
    esp_lcd_panel_io_spi_config_t io_cfg = {};
    io_cfg.cs_gpio_num = PIN_TFT_CS;
    io_cfg.dc_gpio_num = PIN_TFT_DC;
    io_cfg.spi_mode = 0;
    io_cfg.pclk_hz = 40 * 1000 * 1000;
    io_cfg.trans_queue_depth = 10;
    io_cfg.lcd_cmd_bits = 8;
    io_cfg.lcd_param_bits = 8;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST,
                                             &io_cfg, &io));

    // --- ST7789 panel ---
    esp_lcd_panel_dev_config_t panel_cfg = {};
    panel_cfg.reset_gpio_num = PIN_TFT_RST;
    panel_cfg.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
    panel_cfg.bits_per_pixel = 16;
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io, &panel_cfg, &s_panel));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
    // Typical for IPS ST7789 modules; if colors come up inverted or the
    // image is shifted, tune these against the actual panel. CALIBRATE
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(s_panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(s_panel, true));      // landscape
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(s_panel, false, true));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, true));

    pinMode(PIN_TFT_BL, OUTPUT);
    digitalWrite(PIN_TFT_BL, HIGH);

    // --- LVGL ---
    lv_init();
    lv_tick_set_cb([]() -> uint32_t { return millis(); });

    s_lvgl_disp = lv_display_create(DISPLAY_H_RES, DISPLAY_V_RES);

    void* buf1 = heap_caps_malloc(BUF_PIXELS * sizeof(uint16_t),
                                  MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    void* buf2 = heap_caps_malloc(BUF_PIXELS * sizeof(uint16_t),
                                  MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    assert(buf1 && buf2);
    lv_display_set_buffers(s_lvgl_disp, buf1, buf2,
                           BUF_PIXELS * sizeof(uint16_t),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(s_lvgl_disp, flush_cb);

    esp_lcd_panel_io_callbacks_t cbs = {};
    cbs.on_color_trans_done = on_color_trans_done;
    ESP_ERROR_CHECK(esp_lcd_panel_io_register_event_callbacks(io, &cbs, s_lvgl_disp));
}

} // namespace display
