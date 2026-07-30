// task_adc.h — polls the analog channels (~20 Hz) and publishes scaled
// engineering values. In SIM_MODE the ADS1115 is never touched; values come
// from the sim drive cycle instead — everything downstream is identical.

#pragma once

namespace task_adc {
void start();   // creates the task pinned to core 0
}
