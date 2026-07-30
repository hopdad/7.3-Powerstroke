// task_ui.h — LVGL tick/render loop, pinned to core 1. Snapshots sensor
// state, runs the alarm engine, updates widgets. Never performs I/O.

#pragma once

namespace task_ui {
void start();   // creates the task pinned to core 1
}
