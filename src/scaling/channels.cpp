#include "channels.h"

namespace scaling {

float linear(float volts, float v_zero, float v_full, float out_full) {
    float span = v_full - v_zero;
    float out = (volts - v_zero) * (out_full / span);
    if (out < 0.0f) out = 0.0f;          // below-zero readings are offset noise
    return out;
}

float icp_psi(float sensor_volts) {
    return linear(sensor_volts, ICP_V_ZERO, ICP_V_ZERO + ICP_V_SPAN, ICP_PSI_SPAN);
}

float boost_psi(float sensor_volts) {
    return linear(sensor_volts, XDCR_V_ZERO, XDCR_V_FULL, BOOST_PSI_FULL);
}

float fuel_psi(float sensor_volts) {
    return linear(sensor_volts, XDCR_V_ZERO, XDCR_V_FULL, FUEL_PSI_FULL);
}

float map_psi(float sensor_volts) {
    return linear(sensor_volts, MAP_V_ZERO, MAP_V_FULL, MAP_PSI_FULL);
}

// Curves are listed from high volts (cold) to low volts (hot) for NTC
// pull-up dividers. Clamp outside the table; linear interpolation inside.
float interp_curve(const CurvePoint* curve, int len, float volts) {
    if (volts >= curve[0].volts) return curve[0].deg_f;
    if (volts <= curve[len - 1].volts) return curve[len - 1].deg_f;
    for (int i = 1; i < len; i++) {
        if (volts >= curve[i].volts) {
            const CurvePoint& hi = curve[i - 1];   // higher volts, colder
            const CurvePoint& lo = curve[i];
            float t = (volts - lo.volts) / (hi.volts - lo.volts);
            return lo.deg_f + t * (hi.deg_f - lo.deg_f);
        }
    }
    return curve[len - 1].deg_f;   // unreachable, defensive
}

float eot_degf(float sensor_volts) {
    return interp_curve(EOT_CURVE, EOT_CURVE_LEN, sensor_volts);
}

float trans_degf(float sensor_volts) {
    return interp_curve(TRANS_CURVE, TRANS_CURVE_LEN, sensor_volts);
}

} // namespace scaling
