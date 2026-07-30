#include "task_egt.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "../state/sensor_state.h"
#include "sensors_config.h"

#if SIM_MODE
#include "../sim/sim_source.h"
#else
#include "../drivers/max31855.h"
#endif

namespace task_egt {

static void run(void*) {
#if !SIM_MODE
    max31855::init();
#endif

    TickType_t last_wake = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(EGT_PERIOD_MS));

#if SIM_MODE
        sensor_state::publish(CH_EGT, sim::at(millis()).value[CH_EGT], true);
#else
        float deg_f;
        max31855::Fault fault = max31855::read_degf(&deg_f);
        sensor_state::publish(CH_EGT, fault == max31855::Fault::NONE ? deg_f : 0.0f,
                              fault == max31855::Fault::NONE);
#endif
    }
}

void start() {
    static StaticTask_t tcb;
    static StackType_t stack[3072];
    xTaskCreateStaticPinnedToCore(run, "task_egt", 3072, nullptr,
                                  2, stack, &tcb, 0);
}

} // namespace task_egt
