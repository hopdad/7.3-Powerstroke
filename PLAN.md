# Project Plan — OBS Gauge (1996 F-250 7.3L Powerstroke Digital Dash)

This plan operationalizes `Claude.md`. It breaks the four build phases into
concrete milestones, proposes the repository layout, records the technical
decisions the spec leaves open (with rationale), and defines what "done" means
for each phase.

## Guiding constraints (from Claude.md, non-negotiable)

- ESP32-S3 + PlatformIO + Arduino framework, C++
- ADS1115 external ADC only — never the ESP32 internal ADC for measurement
- No OBD-II / ELM327 code paths
- No dynamic allocation in sensor tasks after init
- UI task never blocks on I/O
- US units (°F, psi); estimated scaling constants marked `// CALIBRATE`
- All scaling constants in `sensors_config.h`, all pins in `pins.h`

---

## Key technical decisions

### 1. Display driver: `esp_lcd` (not TFT_eSPI)

**Decision: use `esp_lcd`** (the ESP-IDF LCD component, available under
arduino-esp32 v3.x which is built on IDF 5.x).

Rationale:
- First-class ESP32-S3 support with DMA transfers and transaction-done
  callbacks — this is exactly what LVGL 9's flush callback wants, and it keeps
  the render path off the CPU so `task_ui` stays responsive.
- LVGL 9.x integration with TFT_eSPI is a known pain point (TFT_eSPI targets
  LVGL 8-era APIs and its S3 parallel/SPI configs are fragile); `esp_lcd` has
  official LVGL porting examples for ST7789-class panels.
- `esp_lcd` keeps panel init/config in code (versionable) rather than
  TFT_eSPI's `User_Setup.h` edit-the-library approach, which fights PlatformIO
  dependency management.

Fallback: if arduino-esp32 v3.x causes library friction, LovyanGFX is the
escape hatch (S3-native, DMA, LVGL9-friendly) — but start with `esp_lcd`.

### 2. Shared state: freshest-value snapshot struct, mutex-guarded

A single `SensorState` struct holds the latest value + timestamp + validity
flag per channel. Writers (sensor tasks) take a short-held mutex, copy in
their values, release. Readers (`task_ui`, `task_log`) take the mutex, copy
the whole struct out (it's < 200 bytes), release, then work on their local
copy. Copy-under-mutex keeps hold times in the microseconds, avoids torn
reads, and needs no lock-free cleverness. If profiling ever shows contention,
switch to a double-buffered seqlock — but don't start there.

### 3. IPR duty capture: RMT peripheral

Per the spec, IPR PWM duty is read with the RMT peripheral (or MCPWM capture
as fallback), not the ADC. RMT on the S3 can timestamp edges in hardware;
`task_ipr` converts edge pairs to duty % at ~10 Hz. The IPR signal is 12V PWM
— it needs level shifting/divider hardware before the GPIO (hardware concern,
but the firmware should treat "no edges seen" as a distinct SENSOR_STALE
state, not 0% duty).

### 4. Sim mode: compile flag, same pipeline

`SIM_MODE` is a PlatformIO build flag (`-D SIM_MODE=1`) with two env targets
in `platformio.ini` (`obs-gauge-sim`, `obs-gauge-hw`). In sim mode the sensor
tasks run on schedule but call a `sim_source` instead of touching I2C/SPI —
everything downstream (state struct, alarms, UI, logging) is identical. The
simulator plays a scripted drive cycle: cold idle → warm-up → rev → highway
cruise → WOT pull (fuel pressure sag toward the 45 psi alarm, EGT climb) →
cooldown, so alarm paths get exercised without hardware.

### 5. Data flow

```
ADS1115 ──┐
MAX31855 ─┼─ sensor tasks ── raw → scaled (sensors_config.h) ──► SensorState
RMT/IPR ──┘        (or sim_source when SIM_MODE)                    │ (mutex)
                                                     ┌──────────────┼──────────────┐
                                                 task_ui        task_log       telemetry
                                              (LVGL, core 1)  (SD, batched)   (stub → phase 4)
                                                     │
                                                alarm engine (evaluated on each UI snapshot,
                                                thresholds from alarm_config)
```

---

## Repository layout

```
platformio.ini              # envs: obs-gauge-sim, obs-gauge-hw
include/
  pins.h                    # ALL pin assignments
  sensors_config.h          # ALL scaling constants, // CALIBRATE markers
  alarm_config.h            # warn/critical thresholds per channel
src/
  main.cpp                  # setup: init, task creation, core pinning
  state/sensor_state.{h,cpp}    # SensorState struct + snapshot accessors
  tasks/task_adc.{h,cpp}        # ADS1115 poll ~20 Hz
  tasks/task_egt.{h,cpp}        # MAX31855 ~4 Hz
  tasks/task_ipr.{h,cpp}        # RMT duty capture
  tasks/task_ui.{h,cpp}         # LVGL tick/render, core 1
  tasks/task_log.{h,cpp}        # SD CSV, batched ~1 Hz (phase 4)
  sim/sim_source.{h,cpp}        # SIM_MODE drive-cycle generator
  drivers/display.{h,cpp}       # esp_lcd panel init + LVGL flush glue
  drivers/ads1115.{h,cpp}       # thin wrapper (or vetted lib via lib_deps)
  drivers/max31855.{h,cpp}
  scaling/channels.{h,cpp}      # raw→engineering-unit conversion per channel
  alarms/alarm_engine.{h,cpp}   # threshold eval, state machine (OK/WARN/CRIT)
  ui/screens.{h,cpp}            # LVGL layout: 8-channel main screen
  ui/theme.{h,cpp}              # dark, high-contrast styles
  telemetry/telemetry.h         # interface stub only until phase 4
test/                       # native unit tests: scaling, alarms, sim
docs/
  decisions.md              # running log of choices (driver, buffers, etc.)
  wiring.md                 # grows with phases 2-3
```

---

## Phase 1 — Bench (current phase)

Goal: compile, flash, LVGL UI showing all 8 channels driven by `SIM_MODE`,
with zero hardware beyond MCU + display.

### Milestone 1.1 — Scaffold that compiles
- `platformio.ini` with both envs, LVGL 9.x in `lib_deps`, PSRAM enabled,
  partition/board config for the specific S3 module in use
- `pins.h`, `sensors_config.h`, `alarm_config.h` stubs with every channel
  present and `// CALIBRATE` on all estimated constants
- Empty task skeletons created and pinned (`task_ui` → core 1, sensor tasks →
  core 0); build passes for both envs
- **Done when:** both envs build clean; sim env flashes and boots (serial
  banner + task heartbeat logs)

### Milestone 1.2 — Display up
- `esp_lcd` panel bring-up for the ST7789-class 4" panel; LVGL 9 display
  driver with double buffers in PSRAM, DMA flush
- LVGL "hello" screen at a stable frame rate; record measured FPS in
  `docs/decisions.md`
- **Done when:** panel renders LVGL demo content with no tearing, UI task
  holds rate with sensor task skeletons running

### Milestone 1.3 — State, scaling, sim
- `SensorState` + snapshot API; scaling functions for all 8 channels from
  `sensors_config.h` (ICP 0.5V≈0 psi / ~3.2V≈3000 psi, thermistor curves for
  EOT/trans as table-interpolation, linear transducers for boost/fuel)
- `sim_source` drive cycle wired into the sensor tasks under `SIM_MODE`
- Native unit tests for scaling math and the sim generator (runs on host,
  `pio test -e native`)
- **Done when:** serial dump shows all 8 channels updating with plausible
  values through a full sim cycle; scaling tests pass

### Milestone 1.4 — Gauge UI, all 8 channels
- Main screen: numeric readout + bar/arc per channel, dark theme, sized for
  sunlight/at-a-glance reading; layout priority to ICP, EGT, fuel pressure,
  EOT (the "7.3 fails silently" channels)
- Stale-data indication (value grayed/dashes when a channel's timestamp ages
  out) — distinct from a zero reading
- **Done when:** all 8 channels render live from sim; UI never blocks (no
  I/O on the UI task), frame rate stays stable

### Milestone 1.5 — Alarm engine
- Per-channel OK/WARN/CRIT thresholds from `alarm_config.h`; hysteresis so
  alarms don't flicker at the boundary
- Visual takeover on CRIT (full-screen or banner alert — channel, value,
  threshold), WARN as in-place highlight
- Sim cycle deliberately trips fuel-pressure WARN/CRIT during the WOT segment
- **Done when:** alarms trigger and clear correctly through the sim cycle;
  unit tests cover threshold + hysteresis logic

**Phase 1 exit criteria:** `obs-gauge-sim` flashed on bench hardware runs the
full drive cycle indefinitely: 8 live gauges, alarms firing and clearing, no
watchdog resets, no heap growth after init (verify with
`uxTaskGetStackHighWaterMark` / `esp_get_free_heap_size` logged at 0.1 Hz).

---

## Phase 2 — Own sensors (EGT, boost, fuel pressure)

No truck wiring touched; sensors on the bench first, then plumbed.

- Real ADS1115 driver path: continuous conversion or triggered reads at
  ~20 Hz across channels; verify I2C @ 0x48, plan for 0x49 second unit
- MAX31855 on SPI: fault bits (open thermocouple, short) surfaced as sensor
  validity, not bogus temps
- Bench calibration pass: known pressure source / boiling-ice-bath checks;
  update `sensors_config.h`, remove `// CALIBRATE` only on verified constants
- Brownout/restart behavior: confirm clean reboot to live gauges in < 3 s
- **Exit:** three live channels tracking reality within agreed tolerance;
  remaining channels still simming (mixed real/sim must work — per-channel
  source selection, not one global switch)

## Phase 3 — PCM taps (ICP, EOT, IPR, trans temp)

- Input protection verified before connection (divider/clamp per channel —
  document in `docs/wiring.md`); PCM signals are not to be loaded down
- IPR RMT capture against the real 12V PWM (after level shifting); validate
  duty readings against expected idle (~8-14%) and key-on values
- ICP cross-check: does scaled ICP behave per IH spec at idle (~500-700 psi)
  and free rev? EOT/trans thermistor curves calibrated against IR thermometer
- **Exit:** all 8 channels real; sim retained as a build target forever (it's
  the regression rig)

## Phase 4 — Logging + wireless

- `task_log`: CSV to SD, timestamped, batched writes ~1 Hz, ring buffer so
  sensor tasks never wait on the card; file rotation; survives card removal
- Log includes alarm events as flagged rows
- Telemetry: implement the phase-1 stub — likely BLE GATT first (phone in
  cab), WiFi AP + simple stream as alternative; decide when we get here and
  record in `docs/decisions.md`
- **Exit:** multi-hour drive log with no dropped batches; live stream to a
  phone while logging

---

## Risks / open items

| Risk | Mitigation |
|---|---|
| ADS1115 at 4 ch × 20 Hz is near its practical mux+settle limit (860 SPS single-shot budget) | Profile early in phase 2; second unit @ 0x49 or reduced per-channel rate for slow signals (temps don't need 20 Hz) |
| Exact display controller (ST7789 vs GC9A01-class) unknown until hardware in hand | `drivers/display.cpp` isolates panel init; decision + timing params go in `docs/decisions.md` |
| Thermistor curves for EOT / E4OD test-port NTC are guesses until measured | Table-driven interpolation from day one; tables are data, not code |
| PCM tap loading / grounding offsets between PCM ground and gauge ground | Phase 3 gate: measure before connecting; differential reading on ADS1115 is available if offsets show up |
| Shared SPI bus (display + SD) contention in phase 4 | Separate CS per spec; if flush stutter appears during logging, move SD to its own SPI host (S3 has spare) |

## Immediate next steps

1. Milestone 1.1: create `platformio.ini`, `pins.h`, `sensors_config.h`,
   `alarm_config.h`, task skeletons — get both envs compiling.
2. Milestone 1.2: display bring-up (needs the actual panel's controller ID
   and pinout — first hardware-dependent step).
3. Milestones 1.3 → 1.5 in order; native tests land with 1.3.
