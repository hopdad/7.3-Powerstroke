# 7.3-Powerstroke — OBS Gauge

ESP32-S3 gauge cluster / datalogger for a 1996 F-250 7.3L Powerstroke (HEUI).
Project spec lives in [Claude.md](Claude.md), roadmap in [PLAN.md](PLAN.md),
decision log in [docs/decisions.md](docs/decisions.md).

## Status: Phase 1 (bench)

Full sim-mode pipeline: 8 channels (ICP, EOT, MAP, IPR, EGT, boost, fuel,
trans) rendered with LVGL 9 on a 320x240 ST7789 panel, driven by a scripted
120 s drive cycle that exercises the warn/critical alarm paths. No sensor
hardware required beyond MCU + display.

## Build

```sh
# bench build with simulated sensors (default env)
pio run -e obs-gauge-sim -t upload

# live-sensor build (phases 2-3)
pio run -e obs-gauge-hw -t upload

# host-side unit tests (scaling, alarms, sim)
pio test -e native
```

## Layout

```
include/pins.h              all pin assignments
include/sensors_config.h    all scaling constants (// CALIBRATE = estimate)
include/alarm_config.h      warn/critical thresholds
src/scaling, src/alarms,
src/sim                     pure logic, host-testable
src/state                   mutex-guarded shared snapshot
src/drivers                 ADS1115, MAX31855, esp_lcd/LVGL display
src/tasks                   FreeRTOS tasks (adc, egt, ipr, ui)
src/ui                      LVGL screens + theme
test/test_native            unit tests
```
