#include "sim_source.h"

#include <math.h>

namespace sim {

// Piecewise-linear keyframes over one 120 s cycle. Time in seconds.
struct Key { float t; float v; };

// Segments: 0-15 cold idle | 15-40 warm-up | 40-55 revs | 55-85 highway
//           85-100 WOT pull | 100-120 cooldown to idle
static const Key K_ICP[] = {
    { 0, 650 }, { 15, 650 }, { 40, 620 }, { 42, 1200 }, { 46, 700 },
    { 49, 1250 }, { 52, 700 }, { 55, 650 }, { 60, 950 }, { 85, 950 },
    { 88, 2600 }, { 96, 2850 }, { 100, 1000 }, { 105, 650 }, { 120, 650 },
};
// Wrap-continuous (starts and ends at warm idle); the first-cycle cold
// start is a decaying offset on total time since boot, applied in at().
static const Key K_EGT[] = {
    { 0, 450 }, { 15, 430 }, { 40, 550 }, { 45, 750 }, { 52, 620 },
    { 55, 600 }, { 62, 850 }, { 85, 870 }, { 92, 1230 }, { 98, 1300 },
    { 101, 950 }, { 108, 600 }, { 120, 450 },
};
static const Key K_BOOST[] = {
    { 0, 0 }, { 40, 0 }, { 42, 3 }, { 46, 1 }, { 49, 3.5f }, { 55, 0.5f },
    { 60, 8 }, { 85, 8 }, { 88, 18 }, { 93, 22 }, { 99, 21 },
    { 101, 4 }, { 105, 0 }, { 120, 0 },
};
static const Key K_FUEL[] = {
    { 0, 62 }, { 55, 61 }, { 60, 58 }, { 85, 58 }, { 88, 50 },
    { 91, 46 }, { 94, 43 }, { 98, 44 }, { 100, 55 }, { 103, 61 }, { 120, 62 },
};
static const Key K_MAP[] = {   // psia ≈ atmo + boost
    { 0, 14.7f }, { 40, 14.7f }, { 60, 22.7f }, { 85, 22.7f },
    { 93, 36.7f }, { 99, 35.7f }, { 101, 18.7f }, { 105, 14.7f }, { 120, 14.7f },
};
static const Key K_IPR[] = {
    { 0, 14 }, { 15, 12 }, { 40, 11 }, { 42, 30 }, { 46, 14 },
    { 49, 32 }, { 52, 14 }, { 55, 12 }, { 60, 22 }, { 85, 22 },
    { 88, 52 }, { 96, 58 }, { 100, 24 }, { 105, 12 }, { 120, 12 },
};
// Warm-up channels: keyed across the cycle but blended with a long-term
// warm-up ramp below so they don't snap cold at each wrap.
static const Key K_EOT[] = {
    { 0, 0 }, { 55, 8 }, { 85, 10 }, { 96, 22 }, { 104, 12 }, { 120, 4 },
};   // delta above base warm-up temp
static const Key K_TRANS[] = {
    { 0, 0 }, { 55, 5 }, { 85, 8 }, { 100, 14 }, { 110, 8 }, { 120, 4 },
};

static float interp(const Key* k, int len, float t) {
    if (t <= k[0].t) return k[0].v;
    if (t >= k[len - 1].t) return k[len - 1].v;
    for (int i = 1; i < len; i++) {
        if (t <= k[i].t) {
            float f = (t - k[i - 1].t) / (k[i].t - k[i - 1].t);
            return k[i - 1].v + f * (k[i].v - k[i - 1].v);
        }
    }
    return k[len - 1].v;
}

#define INTERP(K, t) interp(K, (int)(sizeof(K) / sizeof(K[0])), t)

// Small deterministic wobble so gauges look alive; pure function of t.
static float noise(float t, float freq, float amp) {
    return amp * (0.6f * sinf(t * freq) + 0.4f * sinf(t * freq * 2.7f + 1.3f));
}

// Asymptotic warm-up from cold start toward operating temp, across cycles.
static float warmup(float t_total_s, float cold, float hot, float tau_s) {
    return hot + (cold - hot) * expf(-t_total_s / tau_s);
}

SimFrame at(uint32_t t_ms) {
    float t_total = t_ms / 1000.0f;                      // since boot
    float t = fmodf(t_total, CYCLE_MS / 1000.0f);        // within cycle

    SimFrame f;
    f.value[CH_ICP]   = INTERP(K_ICP, t)   + noise(t_total, 5.1f, 18.0f);
    // EGT responds in seconds: cold-start offset decays over the first
    // ~minute since boot, then the cycle runs warm.
    f.value[CH_EGT]   = INTERP(K_EGT, t)   + noise(t_total, 1.7f, 8.0f)
                        - 200.0f * expf(-t_total / 20.0f);
    f.value[CH_BOOST] = INTERP(K_BOOST, t) + noise(t_total, 3.3f, 0.15f);
    f.value[CH_FUEL]  = INTERP(K_FUEL, t)  + noise(t_total, 2.1f, 0.4f);
    f.value[CH_MAP]   = INTERP(K_MAP, t)   + noise(t_total, 3.9f, 0.2f);
    f.value[CH_IPR]   = INTERP(K_IPR, t)   + noise(t_total, 4.3f, 0.6f);

    // EOT: 60 °F cold → ~192 °F operating over ~4 min, plus load delta.
    f.value[CH_EOT]   = warmup(t_total, 60.0f, 192.0f, 240.0f)
                        + INTERP(K_EOT, t) + noise(t_total, 0.7f, 0.6f);
    // Trans: slower warm-up to ~175 °F.
    f.value[CH_TRANS] = warmup(t_total, 60.0f, 175.0f, 360.0f)
                        + INTERP(K_TRANS, t) + noise(t_total, 0.5f, 0.5f);

    if (f.value[CH_BOOST] < 0.0f) f.value[CH_BOOST] = 0.0f;
    return f;
}

} // namespace sim
