// OBS Gauge — 1996 F-250 7.3L Powerstroke digital dash
//
// setup() initializes shared state and the display, then hands everything
// to FreeRTOS tasks:
//   task_adc (core 0) — analog channels via ADS1115 (or sim), ~20 Hz
//   task_egt (core 0) — MAX31855 thermocouple (or sim), ~4 Hz
//   task_ipr (core 0) — IPR PWM duty capture (or sim), ~10 Hz
//   task_ui  (core 1) — LVGL render + alarm engine, ~20 Hz
// loop() only emits a low-rate health heartbeat (heap / stack watermarks)
// used to verify the no-heap-growth phase 1 exit criterion.

#include <Arduino.h>

#include "state/sensor_state.h"
#include "drivers/display.h"
#include "tasks/task_adc.h"
#include "tasks/task_egt.h"
#include "tasks/task_ipr.h"
#include "tasks/task_ui.h"
#include "telemetry/telemetry.h"

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("OBS Gauge — 7.3L Powerstroke digital dash");
#if SIM_MODE
    Serial.println("build: SIM_MODE (bench, no sensor hardware)");
#else
    Serial.println("build: HW (live sensors)");
#endif

    sensor_state::init();
    display::init();
    telemetry::init();

    task_ui::start();
    task_adc::start();
    task_egt::start();
    task_ipr::start();

    Serial.println("[main] tasks started");
}

void loop() {
    // Health heartbeat @ 0.1 Hz. Free heap must be flat after the first
    // minute — any downward trend fails the phase 1 exit criteria.
    static uint32_t last = 0;
    if (millis() - last >= 10000) {
        last = millis();
        Serial.printf("[health] up=%lus heap_free=%u heap_min=%u psram_free=%u\n",
                      (unsigned long)(millis() / 1000),
                      (unsigned)ESP.getFreeHeap(),
                      (unsigned)ESP.getMinFreeHeap(),
                      (unsigned)ESP.getFreePsram());
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
}
