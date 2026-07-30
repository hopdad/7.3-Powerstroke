// ads1115.h — minimal ADS1115 driver: single-shot, single-ended reads with
// OS-bit polling. Deliberately thin (no library dependency): we need exactly
// one mode, and the timing budget matters more than features.
//
// PGA fixed at ±4.096 V (inputs are divided to 0-2.5 V), data rate 250 SPS
// so a conversion completes in ~4 ms and 6 channels fit comfortably in the
// 50 ms ADC task period.

#pragma once

#include <stdint.h>

namespace ads1115 {

// One-time bus + presence check. Returns bitmask of units found (bit 0 =
// 0x48, bit 1 = 0x49).
uint8_t init();

// Blocking single-shot read of single-ended channel mux (0-3) on unit
// (0-1). Returns true on success and writes pin voltage to *volts.
// Typical duration ~5 ms; never called from the UI task.
bool read_single(uint8_t unit, uint8_t mux, float* volts);

} // namespace ads1115
