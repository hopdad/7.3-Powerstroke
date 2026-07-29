# OBS Gauge — 1996 F-250 7.3L Powerstroke Digital Dash

ESP32-S3 gauge cluster / datalogger for a '96 7.3 Powerstroke (HEUI). Reads PCM sensor taps and dedicated sensors, renders real-time gauges with LVGL, logs to SD, streams telemetry over BLE/WiFi.

## Why this exists

Factory OBS gauges are non-functional decoration. The 7.3 fails silently without ICP, EOT, EGT, and fuel pressure monitoring. OBD-II on the '96 (J1850 PWM) is too slow and sparse for real-time use — all signals are read directly.

## Hardware

- **MCU:** ESP32-S3 (dual core, PSRAM preferred for LVGL buffers)
- **ADC:** ADS1115 over I2C @ 0x48 (second unit @ 0x49 if channel count requires) — do NOT use ESP32 internal ADC for sensor channels
- **EGT:** MAX31855 (type-K thermocouple, pre-turbo) over SPI
- **Display:** 4" IPS, ST7789/GC9A01-class, SPI, LVGL 9.x
- **Storage:** SD card over SPI (shared bus with display is acceptable; separate CS)
- **Power:** automotive buck + TVS protection (hardware concern only — firmware should handle brownout restart gracefully)

## Signal map

| Channel | Source | Type | Notes |
|---|---|---|---|
| ICP | PCM tap | 0-5V analog | Injection control pressure; scale per IH spec (0.5V ≈ 0 psi, ~3.2V ≈ 3000 psi typical) |
| EOT | PCM tap | analog (thermistor curve) | Engine oil temp |
| MAP | PCM tap | 0-5V analog | Reference only; boost comes from dedicated sensor |
| IPR | PCM tap | PWM duty cycle | Read with pulse counting / RMT, not ADC |
| EGT | MAX31855 | SPI | Pre-turbo thermocouple |
| Boost | dedicated transducer | 0-5V, 0-30 psi | |
| Fuel pressure | dedicated transducer | 0-5V, 0-100 psi | Alarm below ~45 psi at WOT |
| Trans temp | NTC in E4OD test port | analog | |

Scaling constants live in one header (`sensors_config.h`) — expect calibration tweaks against real hardware.

## Architecture

- FreeRTOS task per sensor group:
  - `task_adc` — polls ADS1115 channels, ~20 Hz
  - `task_egt` — MAX31855, ~4 Hz
  - `task_ipr` — PWM duty capture
  - `task_ui` — LVGL tick/render on core 1
  - `task_log` — SD writes, batched, ~1 Hz
- Sensor tasks write to a single shared state struct guarded by a mutex (or use a freshest-value atomic pattern). UI and logger read only.
- Alarm engine: per-channel warn/critical thresholds, visual alert on display. Thresholds in config, not hardcoded.
- BLE/WiFi telemetry: stub the interface now, implement in phase 4.

## Build phases

1. **Bench (current):** compile, flash, LVGL UI working with **simulated sensor inputs**. Build a `SIM_MODE` compile flag that feeds realistic fake values (idle → rev → highway patterns) so the whole pipeline runs with zero hardware attached beyond MCU + display.
2. **Own sensors:** EGT, boost, fuel pressure live. No truck wiring touched.
3. **PCM taps:** ICP, EOT, IPR, trans temp.
4. **Logging + wireless:** SD logs (CSV, timestamped), BLE or WiFi AP streaming.

## Toolchain & conventions

- PlatformIO, Arduino framework, ESP32-S3 target
- LVGL 9.x via lib_deps; display driver via TFT_eSPI or esp_lcd — pick one and document why
- C++ (not C), no dynamic allocation in sensor tasks after init
- Units: temps in °F, pressure in psi (US units throughout)
- UI: dark theme, high contrast, readable in sunlight and at a glance while driving. No decorative clutter, no emoji. Numeric readouts + bar/arc indicators. Critical alarms take over visually.
- Keep pin assignments in one `pins.h`

## What NOT to do

- No OBD-II / ELM327 code paths
- No internal ESP32 ADC for measurement channels
- Don't block the UI task with I/O
- Don't invent scaling constants as final — mark them `// CALIBRATE` where they're estimates

## Current status

Fresh repo. Start with phase 1: project scaffold, SIM_MODE, LVGL layout with all 8 channels displayed.
