# Decision log

Running record of technical choices and the reasoning behind them. Newest at
the bottom. Reverse a decision by adding a new entry, not by editing history.

## D1 — Display driver: esp_lcd (2026-07)

Chose the ESP-IDF `esp_lcd` component over TFT_eSPI. DMA + trans-done
callbacks fit LVGL 9's flush model, ST7789 support is first-party, and panel
config lives in versioned code instead of TFT_eSPI's edit-the-library
`User_Setup.h`. Fallback if the platform fights us: LovyanGFX.

## D2 — Platform: pioarduino (Arduino core 3.x) (2026-07)

The stock PlatformIO `espressif32` platform is frozen at Arduino core 2.x
(IDF 4.4). `esp_lcd`'s current API and the S3 fixes we want live in core 3.x
(IDF 5.x), which is published for PlatformIO by the pioarduino project. Pinned
via the `stable` release URL in `platformio.ini`.

## D3 — Shared state: copy-under-mutex snapshots (2026-07)

One `SensorSnapshot` struct (< 200 bytes). Writers publish one channel at a
time; readers copy the whole struct under the mutex and work on the copy.
Hold times are microseconds. Upgrade path if profiling ever shows contention:
double-buffered seqlock.

## D4 — IPR duty: GPIO ISR edge capture, not RMT (2026-07)

PLAN.md originally called for RMT. Arduino core 3.x's RMT receive API is
frame-oriented (bounded symbol buffers) and awkward for continuous duty
measurement. A CHANGE-edge ISR timestamping with `esp_timer` (1 µs
resolution) is accurate to ~0.05% duty at the IPR's ~400-500 Hz and is far
simpler. Duty is averaged across all periods in each 100 ms window. RMT or
MCPWM capture remains the upgrade path if ISR jitter becomes measurable.
Zero edges in a window publishes *invalid* (renders stale), never 0%.

## D5 — LVGL buffers: partial, internal DMA RAM (2026-07)

PLAN.md said "double buffers in PSRAM". Corrected at implementation time:
SPI DMA from PSRAM has restrictions on the S3 (cache-line alignment, EDMA
paths) and full-frame buffering isn't needed. Using two 1/8-screen buffers in
internal `MALLOC_CAP_DMA` RAM (2 × 19 KB at 320x240); LVGL renders into one
while the other flushes. PSRAM stays available for LVGL heap growth later if
the UI gets richer.

## D6 — Panel geometry assumption (2026-07)

Code targets 320x240 landscape ST7789 until the actual 4" panel is in hand.
If the panel turns out to be 480x320, the controller is likely ST7796 —
change `DISPLAY_H_RES/V_RES` in `display.h` and swap
`esp_lcd_new_panel_st7789` for the ST7796 vendor init. Invert/mirror/gap
settings are marked CALIBRATE in `display.cpp`.

## Open items

- Measure real LVGL FPS on hardware at milestone 1.2 and record here.
- ADS1115 throughput check at phase 2 (6 ch × 20 Hz across two units).
- Phase 4 transport decision: BLE GATT vs WiFi AP stream.
