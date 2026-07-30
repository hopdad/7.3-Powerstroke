// alarm_engine.h — per-channel OK/WARN/CRIT state machine with hysteresis.
// Pure code: no Arduino/FreeRTOS. Thresholds come from alarm_config.h.
//
// Escalation (OK→WARN, WARN→CRIT) happens the moment a threshold is crossed.
// De-escalation requires the value to retreat past the threshold by the
// channel's hysteresis amount, so alarms don't flicker at the boundary.
// Invalid/stale readings hold the previous level rather than bouncing to OK.

#pragma once

#include <stdint.h>
#include "alarm_config.h"

enum class AlarmLevel : uint8_t { OK = 0, WARN = 1, CRIT = 2 };

class AlarmEngine {
public:
    AlarmEngine();

    // Feed the latest value for one channel; returns the (possibly updated)
    // level. valid=false holds the current level.
    AlarmLevel update(Channel ch, float value, bool valid);

    AlarmLevel level(Channel ch) const { return levels_[ch]; }

    // Highest level across all channels — drives the UI takeover.
    AlarmLevel worst() const;

    void reset();

private:
    // Level the raw value maps to with no hysteresis applied.
    static AlarmLevel classify(const AlarmThresholds& t, float value);
    // True if value has retreated past threshold by at least hyst.
    static bool cleared_high(float value, float threshold, float hyst);
    static bool cleared_low(float value, float threshold, float hyst);

    AlarmLevel levels_[CH_COUNT];
};
