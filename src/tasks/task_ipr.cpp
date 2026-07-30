#include "task_ipr.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "../state/sensor_state.h"
#include "sensors_config.h"
#include "pins.h"

#if SIM_MODE
#include "../sim/sim_source.h"
#endif

namespace task_ipr {

#if !SIM_MODE
// ISR accumulators: high time and period sums since last harvest.
static volatile uint32_t s_last_rise_us = 0;
static volatile uint32_t s_last_fall_us = 0;
static volatile uint64_t s_high_sum_us = 0;
static volatile uint64_t s_period_sum_us = 0;
static volatile uint32_t s_period_count = 0;

static void IRAM_ATTR on_edge() {
    uint32_t now = (uint32_t)esp_timer_get_time();
    if (digitalRead(PIN_IPR_IN)) {
        if (s_last_rise_us != 0) {
            s_period_sum_us += now - s_last_rise_us;
            s_high_sum_us += s_last_fall_us - s_last_rise_us;
            s_period_count++;
        }
        s_last_rise_us = now;
    } else {
        s_last_fall_us = now;
    }
}
#endif

static void run(void*) {
#if !SIM_MODE
    pinMode(PIN_IPR_IN, INPUT);
    attachInterrupt(digitalPinToInterrupt(PIN_IPR_IN), on_edge, CHANGE);
#endif

    TickType_t last_wake = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(IPR_PERIOD_MS));

#if SIM_MODE
        sensor_state::publish(CH_IPR, sim::at(millis()).value[CH_IPR], true);
#else
        // Harvest atomically relative to the ISR.
        noInterrupts();
        uint64_t high = s_high_sum_us;
        uint64_t period = s_period_sum_us;
        uint32_t count = s_period_count;
        s_high_sum_us = 0;
        s_period_sum_us = 0;
        s_period_count = 0;
        interrupts();

        if (count == 0 || period == 0) {
            // No edges this window: key-off, wiring fault, or DC signal.
            // Publish invalid — downstream renders SENSOR_STALE, not 0%.
            sensor_state::publish(CH_IPR, 0.0f, false);
        } else {
            sensor_state::publish(CH_IPR, 100.0f * (float)high / (float)period, true);
        }
#endif
    }
}

void start() {
    static StaticTask_t tcb;
    static StackType_t stack[3072];
    xTaskCreateStaticPinnedToCore(run, "task_ipr", 3072, nullptr,
                                  2, stack, &tcb, 0);
}

} // namespace task_ipr
