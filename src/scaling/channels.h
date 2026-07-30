// channels.h — raw reading → engineering units, per channel. Pure code.
//
// All functions take the voltage AT THE SENSOR (callers undo the input
// divider first via pin_volts / ANALOG_DIVIDER_RATIO). Outputs are US units
// per project convention: psi and °F.

#pragma once

#include "sensors_config.h"

namespace scaling {

// Linear channels
float icp_psi(float sensor_volts);
float boost_psi(float sensor_volts);
float fuel_psi(float sensor_volts);
float map_psi(float sensor_volts);

// Table-interpolated thermistor channels
float eot_degf(float sensor_volts);
float trans_degf(float sensor_volts);

// Generic helpers (exposed for tests)
float linear(float volts, float v_zero, float v_full, float out_full);
float interp_curve(const CurvePoint* curve, int len, float volts);

// Undo the analog front-end divider: ADS1115 pin volts → sensor volts.
inline float pin_to_sensor_volts(float pin_volts) {
    return pin_volts / ANALOG_DIVIDER_RATIO;
}

} // namespace scaling
