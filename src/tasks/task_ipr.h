// task_ipr.h — IPR PWM duty capture.
//
// Implementation: GPIO edge ISR timestamping with esp_timer (µs resolution),
// duty recomputed at ~10 Hz. The plan originally said RMT; the Arduino core
// 3.x RMT receive API is built around bounded symbol frames and is awkward
// for continuous duty measurement, while ISR edge capture is µs-accurate at
// IPR frequencies (~400-500 Hz) and dead simple. RMT/MCPWM-capture remains
// the upgrade path if ISR jitter ever matters. See docs/decisions.md.
//
// "No edges seen" publishes invalid (SENSOR_STALE downstream), never 0%.

#pragma once

namespace task_ipr {
void start();   // creates the task pinned to core 0
}
