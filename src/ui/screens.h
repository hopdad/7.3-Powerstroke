// screens.h — LVGL layout: 4x2 grid of gauge cells (name, numeric readout,
// bar), plus a critical-alarm overlay that takes over the display.
//
// Layout priority per spec: the "7.3 fails silently" channels (ICP, EGT,
// FUEL, EOT) get the top row.
//
// All widget updates happen on the UI task; this module owns no locking.

#pragma once

#include "../state/sensor_state.h"
#include "../alarms/alarm_engine.h"

namespace screens {

// Build the main screen. Call once after display::init().
void create();

// Push a fresh snapshot into the widgets. alarm levels drive per-cell
// colors; a CRIT anywhere raises the takeover overlay.
void update(const SensorSnapshot& snap, const AlarmEngine& alarms, uint32_t now_ms);

} // namespace screens
