// alarm_config.h — per-channel warn/critical thresholds. Pure header.
//
// NaN means "no threshold on this side". Hysteresis is the amount a value
// must retreat past a threshold before the alarm level drops, so alarms
// don't flicker at the boundary.
//
// Values are sane starting points for a stock-ish 7.3 — CALIBRATE / tune
// to taste once real data is flowing.

#pragma once

#include <math.h>
#include "sensors_config.h"

struct AlarmThresholds {
    float warn_low;
    float warn_high;
    float crit_low;
    float crit_high;
    float hysteresis;
};

#define ALARM_NONE NAN

// Order must match enum Channel.
static const AlarmThresholds ALARM_CONFIG[CH_COUNT] = {
    // warn_low     warn_high   crit_low    crit_high   hyst
    { ALARM_NONE,   3200.0f,    ALARM_NONE, 3500.0f,    50.0f  },  // ICP — sustained max-ICP suggests a problem
    { ALARM_NONE,   230.0f,     ALARM_NONE, 250.0f,     4.0f   },  // EOT
    { ALARM_NONE,   ALARM_NONE, ALARM_NONE, ALARM_NONE, 0.0f   },  // MAP — reference only
    { ALARM_NONE,   ALARM_NONE, ALARM_NONE, ALARM_NONE, 0.0f   },  // IPR — info only
    { ALARM_NONE,   1200.0f,    ALARM_NONE, 1350.0f,    25.0f  },  // EGT pre-turbo
    { ALARM_NONE,   25.0f,      ALARM_NONE, 30.0f,      1.0f   },  // Boost
    { 50.0f,        ALARM_NONE, 45.0f,      ALARM_NONE, 2.0f   },  // Fuel — spec: alarm below ~45 psi at WOT
    { ALARM_NONE,   200.0f,     ALARM_NONE, 220.0f,     4.0f   },  // Trans
};
