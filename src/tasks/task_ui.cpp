#include "task_ui.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lvgl.h>

#include "../state/sensor_state.h"
#include "../alarms/alarm_engine.h"
#include "../ui/screens.h"
#include "sensors_config.h"

namespace task_ui {

static AlarmEngine s_alarms;

static void run(void*) {
    screens::create();

    TickType_t last_wake = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(UI_PERIOD_MS));

        uint32_t now = millis();
        SensorSnapshot snap = sensor_state::snapshot();

        for (int i = 0; i < CH_COUNT; i++) {
            Channel ch = (Channel)i;
            // Stale readings hold their alarm level rather than clearing it.
            s_alarms.update(ch, snap.ch[ch].value,
                            snap.ch[ch].valid && !snap.is_stale(ch, now));
        }

        screens::update(snap, s_alarms, now);
        lv_timer_handler();
    }
}

void start() {
    static StaticTask_t tcb;
    static StackType_t stack[8192];
    xTaskCreateStaticPinnedToCore(run, "task_ui", 8192, nullptr,
                                  3, stack, &tcb, 1);
}

} // namespace task_ui
