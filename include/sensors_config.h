// sensors_config.h — ALL scaling constants and channel wiring live here.
// Pure header: no Arduino/FreeRTOS includes, so host-side tests can use it.
//
// Every constant marked // CALIBRATE is an estimate and must be verified
// against real hardware before it is trusted.

#pragma once

#include <stdint.h>

// ---- Channel identity -------------------------------------------------------

enum Channel : uint8_t {
    CH_ICP = 0,   // injection control pressure (PCM tap)
    CH_EOT,       // engine oil temp (PCM tap, thermistor)
    CH_MAP,       // manifold absolute pressure (PCM tap, reference only)
    CH_IPR,       // IPR duty cycle % (PCM tap, PWM)
    CH_EGT,       // exhaust gas temp, pre-turbo (MAX31855)
    CH_BOOST,     // dedicated transducer, 0-30 psi
    CH_FUEL,      // dedicated transducer, 0-100 psi
    CH_TRANS,     // E4OD trans temp (NTC in test port)
    CH_COUNT
};

// Display metadata (name shown on the gauge, unit string)
struct ChannelMeta {
    const char* name;
    const char* unit;
    float       display_min;   // gauge bar range
    float       display_max;
    uint8_t     decimals;      // digits after the point on the readout
};

// Order must match enum Channel.
static const ChannelMeta CHANNEL_META[CH_COUNT] = {
    { "ICP",   "psi", 0.0f,    3600.0f, 0 },
    { "EOT",   "\xC2\xB0" "F", 50.0f, 280.0f,  0 },
    { "MAP",   "psi", 0.0f,    45.0f,   1 },
    { "IPR",   "%",   0.0f,    100.0f,  0 },
    { "EGT",   "\xC2\xB0" "F", 0.0f,  1600.0f, 0 },
    { "BOOST", "psi", 0.0f,    35.0f,   1 },
    { "FUEL",  "psi", 0.0f,    100.0f,  0 },
    { "TRANS", "\xC2\xB0" "F", 50.0f, 260.0f,  0 },
};

// ---- Analog front end -------------------------------------------------------
// 0-5V sensor outputs are divided down to the ADS1115 input range (ADS powered
// at 3.3V, PGA ±4.096V FSR). Divider: 10k/10k = 0.5. The scale functions take
// the voltage AT THE SENSOR, so tasks multiply the pin reading by the inverse
// ratio first.
static const float ANALOG_DIVIDER_RATIO = 0.5f;   // CALIBRATE against real resistors

// ADS1115 unit / mux assignment.
// Unit 0 @ 0x48, unit 1 @ 0x49.
struct AdcInput { uint8_t unit; uint8_t mux; };   // mux = single-ended channel 0-3

static const uint8_t ADS1115_ADDR[2] = { 0x48, 0x49 };

// Analog channels only (IPR is PWM, EGT is SPI).
static const AdcInput ADC_INPUT_ICP   = { 0, 0 };
static const AdcInput ADC_INPUT_EOT   = { 0, 1 };
static const AdcInput ADC_INPUT_MAP   = { 0, 2 };
static const AdcInput ADC_INPUT_BOOST = { 0, 3 };
static const AdcInput ADC_INPUT_FUEL  = { 1, 0 };
static const AdcInput ADC_INPUT_TRANS = { 1, 1 };

// ---- Poll rates --------------------------------------------------------------
static const uint32_t ADC_PERIOD_MS  = 50;    // ~20 Hz across all analog channels
static const uint32_t EGT_PERIOD_MS  = 250;   // ~4 Hz
static const uint32_t IPR_PERIOD_MS  = 100;   // duty recompute ~10 Hz
static const uint32_t UI_PERIOD_MS   = 50;    // gauge refresh ~20 Hz
static const uint32_t STALE_AFTER_MS = 1500;  // channel older than this renders as stale

// ---- ICP: linear per IH spec ------------------------------------------------
// 0.5V ≈ 0 psi, ~3.2V ≈ 3000 psi.
static const float ICP_V_ZERO  = 0.5f;     // CALIBRATE
static const float ICP_V_SPAN  = 2.7f;     // volts from 0 → 3000 psi  // CALIBRATE
static const float ICP_PSI_SPAN = 3000.0f;

// ---- Linear pressure transducers (0.5-4.5V ratiometric) ----------------------
static const float XDCR_V_ZERO = 0.5f;     // CALIBRATE
static const float XDCR_V_FULL = 4.5f;     // CALIBRATE
static const float BOOST_PSI_FULL = 30.0f;
static const float FUEL_PSI_FULL  = 100.0f;

// ---- MAP (PCM tap, reference only) -------------------------------------------
// Treated as a generic 0-5V ≈ 0-45 psia map until characterized on the truck.
static const float MAP_V_ZERO   = 0.0f;    // CALIBRATE
static const float MAP_V_FULL   = 5.0f;    // CALIBRATE
static const float MAP_PSI_FULL = 45.0f;   // CALIBRATE

// ---- Thermistor curves (voltage at sensor → °F) -------------------------------
// Table-driven linear interpolation; tables are data, not code. These are
// placeholder shapes for a typical NTC in a pull-up divider — replace with
// measured points (IR thermometer / ice bath) during calibration.
struct CurvePoint { float volts; float deg_f; };

static const CurvePoint EOT_CURVE[] = {   // CALIBRATE — placeholder curve
    { 4.60f,  -20.0f },
    { 4.20f,   32.0f },
    { 3.50f,  100.0f },
    { 2.60f,  160.0f },
    { 1.80f,  200.0f },
    { 1.20f,  230.0f },
    { 0.70f,  260.0f },
    { 0.40f,  300.0f },
};
static const int EOT_CURVE_LEN = sizeof(EOT_CURVE) / sizeof(EOT_CURVE[0]);

static const CurvePoint TRANS_CURVE[] = { // CALIBRATE — placeholder curve
    { 4.60f,  -20.0f },
    { 4.20f,   32.0f },
    { 3.40f,  100.0f },
    { 2.40f,  160.0f },
    { 1.60f,  200.0f },
    { 1.00f,  230.0f },
    { 0.55f,  260.0f },
};
static const int TRANS_CURVE_LEN = sizeof(TRANS_CURVE) / sizeof(TRANS_CURVE[0]);
