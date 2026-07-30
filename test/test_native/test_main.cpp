// Host-side unit tests (pio test -e native) for the pure modules:
// scaling math, alarm engine state machine + hysteresis, sim drive cycle.

#include <unity.h>
#include <math.h>

#include "../../src/scaling/channels.h"
#include "../../src/alarms/alarm_engine.h"
#include "../../src/sim/sim_source.h"

void setUp() {}
void tearDown() {}

// ---- scaling ----------------------------------------------------------------

static void test_icp_scaling() {
    // IH spec anchors: 0.5V ≈ 0 psi, 3.2V ≈ 3000 psi
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.0f, scaling::icp_psi(0.5f));
    TEST_ASSERT_FLOAT_WITHIN(5.0f, 3000.0f, scaling::icp_psi(3.2f));
    TEST_ASSERT_FLOAT_WITHIN(5.0f, 1500.0f, scaling::icp_psi(1.85f));
    // below-zero offset noise clamps to 0, not negative psi
    TEST_ASSERT_EQUAL_FLOAT(0.0f, scaling::icp_psi(0.3f));
}

static void test_transducer_scaling() {
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, scaling::boost_psi(0.5f));
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 30.0f, scaling::boost_psi(4.5f));
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 15.0f, scaling::boost_psi(2.5f));

    TEST_ASSERT_FLOAT_WITHIN(0.2f, 0.0f, scaling::fuel_psi(0.5f));
    TEST_ASSERT_FLOAT_WITHIN(0.2f, 100.0f, scaling::fuel_psi(4.5f));
    TEST_ASSERT_FLOAT_WITHIN(0.2f, 50.0f, scaling::fuel_psi(2.5f));
}

static void test_divider_inversion() {
    // 2.5V at the ADS1115 pin through a 0.5 divider is 5.0V at the sensor.
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, scaling::pin_to_sensor_volts(2.5f));
}

static void test_thermistor_interpolation() {
    // Exact table points hit exactly
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 100.0f, scaling::eot_degf(3.50f));
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 230.0f, scaling::eot_degf(1.20f));
    // Between points: interpolated, monotonic (lower volts = hotter for NTC)
    float mid = scaling::eot_degf(3.0f);
    TEST_ASSERT_TRUE(mid > 100.0f && mid < 160.0f);
    // Clamped outside the table
    TEST_ASSERT_EQUAL_FLOAT(-20.0f, scaling::eot_degf(5.0f));
    TEST_ASSERT_EQUAL_FLOAT(300.0f, scaling::eot_degf(0.1f));
    // Monotonic sweep across the whole table
    float prev = scaling::eot_degf(4.8f);
    for (float v = 4.7f; v > 0.2f; v -= 0.05f) {
        float cur = scaling::eot_degf(v);
        TEST_ASSERT_TRUE(cur >= prev);
        prev = cur;
    }
}

// ---- alarm engine -------------------------------------------------------------

static void test_fuel_low_alarm_path() {
    // Fuel: warn_low 50, crit_low 45, hyst 2
    AlarmEngine e;
    TEST_ASSERT_EQUAL((int)AlarmLevel::OK,   (int)e.update(CH_FUEL, 60.0f, true));
    TEST_ASSERT_EQUAL((int)AlarmLevel::WARN, (int)e.update(CH_FUEL, 48.0f, true));
    TEST_ASSERT_EQUAL((int)AlarmLevel::CRIT, (int)e.update(CH_FUEL, 44.0f, true));
    // Inside hysteresis band: still CRIT (45 + 2 = 47 needed to clear)
    TEST_ASSERT_EQUAL((int)AlarmLevel::CRIT, (int)e.update(CH_FUEL, 46.0f, true));
    // Past crit hysteresis but still in warn range → WARN
    TEST_ASSERT_EQUAL((int)AlarmLevel::WARN, (int)e.update(CH_FUEL, 48.0f, true));
    // Inside warn hysteresis (50 + 2 = 52 needed): still WARN
    TEST_ASSERT_EQUAL((int)AlarmLevel::WARN, (int)e.update(CH_FUEL, 51.0f, true));
    TEST_ASSERT_EQUAL((int)AlarmLevel::OK,   (int)e.update(CH_FUEL, 55.0f, true));
}

static void test_egt_high_alarm_path() {
    // EGT: warn_high 1200, crit_high 1350, hyst 25
    AlarmEngine e;
    TEST_ASSERT_EQUAL((int)AlarmLevel::OK,   (int)e.update(CH_EGT, 900.0f, true));
    TEST_ASSERT_EQUAL((int)AlarmLevel::WARN, (int)e.update(CH_EGT, 1250.0f, true));
    TEST_ASSERT_EQUAL((int)AlarmLevel::CRIT, (int)e.update(CH_EGT, 1400.0f, true));
    TEST_ASSERT_EQUAL((int)AlarmLevel::CRIT, (int)e.update(CH_EGT, 1340.0f, true)); // inside hyst
    TEST_ASSERT_EQUAL((int)AlarmLevel::WARN, (int)e.update(CH_EGT, 1300.0f, true));
    TEST_ASSERT_EQUAL((int)AlarmLevel::OK,   (int)e.update(CH_EGT, 1100.0f, true));
}

static void test_crit_to_ok_when_fully_clear() {
    // A big drop straight through both hysteresis bands lands on OK directly.
    AlarmEngine e;
    e.update(CH_EGT, 1400.0f, true);
    TEST_ASSERT_EQUAL((int)AlarmLevel::OK, (int)e.update(CH_EGT, 600.0f, true));
}

static void test_invalid_holds_level() {
    AlarmEngine e;
    e.update(CH_FUEL, 44.0f, true);
    TEST_ASSERT_EQUAL((int)AlarmLevel::CRIT, (int)e.update(CH_FUEL, 999.0f, false));
    TEST_ASSERT_EQUAL((int)AlarmLevel::CRIT, (int)e.level(CH_FUEL));
}

static void test_unalarmed_channel_never_fires() {
    AlarmEngine e;
    TEST_ASSERT_EQUAL((int)AlarmLevel::OK, (int)e.update(CH_MAP, 1e6f, true));
    TEST_ASSERT_EQUAL((int)AlarmLevel::OK, (int)e.update(CH_MAP, -1e6f, true));
}

static void test_worst_aggregation() {
    AlarmEngine e;
    TEST_ASSERT_EQUAL((int)AlarmLevel::OK, (int)e.worst());
    e.update(CH_EGT, 1250.0f, true);
    TEST_ASSERT_EQUAL((int)AlarmLevel::WARN, (int)e.worst());
    e.update(CH_FUEL, 40.0f, true);
    TEST_ASSERT_EQUAL((int)AlarmLevel::CRIT, (int)e.worst());
}

// ---- sim drive cycle -----------------------------------------------------------

static void test_sim_values_finite_and_plausible() {
    for (uint32_t t = 0; t < 2 * sim::CYCLE_MS; t += 250) {
        sim::SimFrame f = sim::at(t);
        for (int c = 0; c < CH_COUNT; c++) {
            TEST_ASSERT_TRUE(isfinite(f.value[c]));
        }
        TEST_ASSERT_TRUE(f.value[CH_ICP] >= 0.0f && f.value[CH_ICP] < 3600.0f);
        TEST_ASSERT_TRUE(f.value[CH_EGT] > 100.0f && f.value[CH_EGT] < 1400.0f);
        TEST_ASSERT_TRUE(f.value[CH_BOOST] >= 0.0f && f.value[CH_BOOST] < 30.0f);
        TEST_ASSERT_TRUE(f.value[CH_FUEL] > 35.0f && f.value[CH_FUEL] < 70.0f);
        TEST_ASSERT_TRUE(f.value[CH_IPR] > 5.0f && f.value[CH_IPR] < 70.0f);
        TEST_ASSERT_TRUE(f.value[CH_EOT] > 50.0f && f.value[CH_EOT] < 240.0f);
        TEST_ASSERT_TRUE(f.value[CH_TRANS] > 50.0f && f.value[CH_TRANS] < 220.0f);
    }
}

static void test_sim_wot_trips_fuel_crit() {
    // During the WOT pull the fuel pressure must sag below the 45 psi
    // critical threshold so the alarm path gets exercised on the bench.
    AlarmEngine e;
    bool tripped = false;
    for (uint32_t t = 0; t < sim::CYCLE_MS; t += 100) {
        e.update(CH_FUEL, sim::at(t).value[CH_FUEL], true);
        if (e.level(CH_FUEL) == AlarmLevel::CRIT) tripped = true;
    }
    TEST_ASSERT_TRUE_MESSAGE(tripped, "sim cycle never tripped fuel CRIT");
    // And idle at cycle start is comfortably OK.
    AlarmEngine e2;
    e2.update(CH_FUEL, sim::at(2000).value[CH_FUEL], true);
    TEST_ASSERT_EQUAL((int)AlarmLevel::OK, (int)e2.level(CH_FUEL));
}

static void test_sim_wot_trips_egt_warn() {
    AlarmEngine e;
    bool warned = false;
    for (uint32_t t = 0; t < sim::CYCLE_MS; t += 100) {
        e.update(CH_EGT, sim::at(t).value[CH_EGT], true);
        if (e.level(CH_EGT) >= AlarmLevel::WARN) warned = true;
    }
    TEST_ASSERT_TRUE_MESSAGE(warned, "sim cycle never reached EGT warn");
}

static void test_sim_cycle_wraps_continuously() {
    // Frame just before the wrap and just after must not teleport (temps
    // warm across cycles by design; check the fast channels).
    sim::SimFrame a = sim::at(sim::CYCLE_MS - 100);
    sim::SimFrame b = sim::at(sim::CYCLE_MS + 100);
    TEST_ASSERT_FLOAT_WITHIN(80.0f, a.value[CH_ICP], b.value[CH_ICP]);
    TEST_ASSERT_FLOAT_WITHIN(60.0f, a.value[CH_EGT], b.value[CH_EGT]);
    TEST_ASSERT_FLOAT_WITHIN(3.0f,  a.value[CH_FUEL], b.value[CH_FUEL]);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_icp_scaling);
    RUN_TEST(test_transducer_scaling);
    RUN_TEST(test_divider_inversion);
    RUN_TEST(test_thermistor_interpolation);
    RUN_TEST(test_fuel_low_alarm_path);
    RUN_TEST(test_egt_high_alarm_path);
    RUN_TEST(test_crit_to_ok_when_fully_clear);
    RUN_TEST(test_invalid_holds_level);
    RUN_TEST(test_unalarmed_channel_never_fires);
    RUN_TEST(test_worst_aggregation);
    RUN_TEST(test_sim_values_finite_and_plausible);
    RUN_TEST(test_sim_wot_trips_fuel_crit);
    RUN_TEST(test_sim_wot_trips_egt_warn);
    RUN_TEST(test_sim_cycle_wraps_continuously);
    return UNITY_END();
}
