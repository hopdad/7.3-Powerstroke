// sensor_state.h — the single shared state struct between sensor tasks
// (writers) and UI/logger (readers).
//
// Pattern: copy-under-mutex snapshots. Writers hold the mutex only long
// enough to store value+timestamp+valid; readers copy the whole struct out
// (< 200 bytes) and work on the local copy. Hold times stay in the
// microseconds. If contention ever shows up in profiling, the upgrade path
// is a double-buffered seqlock — don't start there.

#pragma once

#include <stdint.h>
#include "sensors_config.h"

struct ChannelReading {
    float    value;      // engineering units (psi, °F, %)
    uint32_t ts_ms;      // millis() at last update
    bool     valid;      // sensor fault / not yet read → false
};

struct SensorSnapshot {
    ChannelReading ch[CH_COUNT];

    bool is_stale(Channel c, uint32_t now_ms) const {
        return !ch[c].valid || (now_ms - ch[c].ts_ms) > STALE_AFTER_MS;
    }
};

namespace sensor_state {

void init();

// Writer side (sensor tasks).
void publish(Channel c, float value, bool valid);

// Reader side (UI, logger): full copy out.
SensorSnapshot snapshot();

} // namespace sensor_state
