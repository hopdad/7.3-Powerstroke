#include "alarm_engine.h"

#include <math.h>

AlarmEngine::AlarmEngine() { reset(); }

void AlarmEngine::reset() {
    for (int i = 0; i < CH_COUNT; i++) levels_[i] = AlarmLevel::OK;
}

AlarmLevel AlarmEngine::classify(const AlarmThresholds& t, float v) {
    if ((!isnan(t.crit_high) && v >= t.crit_high) ||
        (!isnan(t.crit_low)  && v <= t.crit_low)) {
        return AlarmLevel::CRIT;
    }
    if ((!isnan(t.warn_high) && v >= t.warn_high) ||
        (!isnan(t.warn_low)  && v <= t.warn_low)) {
        return AlarmLevel::WARN;
    }
    return AlarmLevel::OK;
}

bool AlarmEngine::cleared_high(float v, float threshold, float hyst) {
    return isnan(threshold) || v < threshold - hyst;
}

bool AlarmEngine::cleared_low(float v, float threshold, float hyst) {
    return isnan(threshold) || v > threshold + hyst;
}

AlarmLevel AlarmEngine::update(Channel ch, float value, bool valid) {
    if (!valid) return levels_[ch];   // stale data: hold, don't bounce to OK

    const AlarmThresholds& t = ALARM_CONFIG[ch];
    AlarmLevel current = levels_[ch];
    AlarmLevel raw = classify(t, value);

    // Escalate immediately.
    if (raw > current) {
        levels_[ch] = raw;
        return raw;
    }

    // De-escalate only past hysteresis, one step of thresholds at a time:
    // drop from CRIT once clear of crit±hyst, from WARN once clear of
    // warn±hyst. The raw classification of the new value decides how far
    // down we land.
    if (raw < current) {
        if (current == AlarmLevel::CRIT) {
            if (cleared_high(value, t.crit_high, t.hysteresis) &&
                cleared_low(value, t.crit_low, t.hysteresis)) {
                current = (raw == AlarmLevel::OK &&
                           cleared_high(value, t.warn_high, t.hysteresis) &&
                           cleared_low(value, t.warn_low, t.hysteresis))
                              ? AlarmLevel::OK
                              : AlarmLevel::WARN;
            }
        } else if (current == AlarmLevel::WARN) {
            if (cleared_high(value, t.warn_high, t.hysteresis) &&
                cleared_low(value, t.warn_low, t.hysteresis)) {
                current = AlarmLevel::OK;
            }
        }
        levels_[ch] = current;
    }
    return levels_[ch];
}

AlarmLevel AlarmEngine::worst() const {
    AlarmLevel w = AlarmLevel::OK;
    for (int i = 0; i < CH_COUNT; i++) {
        if (levels_[i] > w) w = levels_[i];
    }
    return w;
}
