#include "task_adc.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "../state/sensor_state.h"
#include "../scaling/channels.h"
#include "sensors_config.h"

#if SIM_MODE
#include "../sim/sim_source.h"
#else
#include "../drivers/ads1115.h"
#endif

namespace task_adc {

#if !SIM_MODE
struct AnalogChannel {
    Channel  ch;
    AdcInput input;
    float (*scale)(float sensor_volts);
};

static const AnalogChannel ANALOG_CHANNELS[] = {
    { CH_ICP,   ADC_INPUT_ICP,   scaling::icp_psi   },
    { CH_EOT,   ADC_INPUT_EOT,   scaling::eot_degf  },
    { CH_MAP,   ADC_INPUT_MAP,   scaling::map_psi   },
    { CH_BOOST, ADC_INPUT_BOOST, scaling::boost_psi },
    { CH_FUEL,  ADC_INPUT_FUEL,  scaling::fuel_psi  },
    { CH_TRANS, ADC_INPUT_TRANS, scaling::trans_degf},
};
static const int N_ANALOG = sizeof(ANALOG_CHANNELS) / sizeof(ANALOG_CHANNELS[0]);
#endif

static void run(void*) {
#if !SIM_MODE
    uint8_t found = ads1115::init();
    Serial.printf("[adc] ADS1115 units found: 0x%02x\n", found);
#endif

    TickType_t last_wake = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(ADC_PERIOD_MS));

#if SIM_MODE
        sim::SimFrame f = sim::at(millis());
        sensor_state::publish(CH_ICP,   f.value[CH_ICP],   true);
        sensor_state::publish(CH_EOT,   f.value[CH_EOT],   true);
        sensor_state::publish(CH_MAP,   f.value[CH_MAP],   true);
        sensor_state::publish(CH_BOOST, f.value[CH_BOOST], true);
        sensor_state::publish(CH_FUEL,  f.value[CH_FUEL],  true);
        sensor_state::publish(CH_TRANS, f.value[CH_TRANS], true);
#else
        for (int i = 0; i < N_ANALOG; i++) {
            const AnalogChannel& a = ANALOG_CHANNELS[i];
            float pin_volts;
            bool ok = ads1115::read_single(a.input.unit, a.input.mux, &pin_volts);
            float value = ok ? a.scale(scaling::pin_to_sensor_volts(pin_volts)) : 0.0f;
            sensor_state::publish(a.ch, value, ok);
        }
#endif
    }
}

void start() {
    static StaticTask_t tcb;
    static StackType_t stack[4096];
    xTaskCreateStaticPinnedToCore(run, "task_adc", 4096, nullptr,
                                  2, stack, &tcb, 0);
}

} // namespace task_adc
