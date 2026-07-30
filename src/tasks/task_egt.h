// task_egt.h — MAX31855 thermocouple reads at ~4 Hz. Fault bits publish as
// invalid, never as a fake temperature. SIM_MODE reads the drive cycle.

#pragma once

namespace task_egt {
void start();   // creates the task pinned to core 0
}
