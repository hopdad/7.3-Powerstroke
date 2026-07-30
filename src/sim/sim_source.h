// sim_source.h — SIM_MODE drive-cycle generator. Pure code (function of time),
// so the exact same values can be unit-tested on the host.
//
// The cycle is a 120 s loop: cold idle → warm-up → free revs → highway
// cruise → WOT pull (fuel pressure sags through the 45 psi critical alarm,
// EGT climbs through the 1200 °F warning) → cooldown. Temps (EOT/trans) warm
// monotonically over the first cycles instead of resetting, like a real
// engine would.

#pragma once

#include <stdint.h>
#include "sensors_config.h"

namespace sim {

struct SimFrame {
    float value[CH_COUNT];   // engineering units per channel
};

// Milliseconds since boot → one frame of all 8 channels.
SimFrame at(uint32_t t_ms);

// Cycle length, exposed for tests.
static const uint32_t CYCLE_MS = 120000;

} // namespace sim
