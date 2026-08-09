/*
 * The power state machine. No ADC, no cell, no charger: the fake Io below
 * counts what was asked for and the clock is injected, so a five-minute charge
 * window is tested without waiting five minutes.
 *
 * The shape to keep in mind: readings are sticky rather than queued. One
 * post_sample() can walk the machine down several rungs of the ladder, because
 * on battery there is no second sample coming — a wake is a fresh boot and the
 * device is awake for about two seconds.
 */

#include <zephyr/ztest.h>

#include "power_fsm.hpp"

using namespace tk;
using State = PowerFsm::State;

namespace
{

/** Records everything the machine did to the world outside itself. */
class FakePowerIo : public PowerIo
{
public:
    int publishes = 0;
    PowerState last_state = PowerState::UNKNOWN;
    uint16_t last_mv = 0;
    bool last_usb = false;

    int opens = 0;
    int closes = 0;

    void publish(PowerState state, uint16_t mv, bool usb) override
    {
        publishes++;
        last_state = state;
        last_mv = mv;
        last_usb = usb;
    }

    void open_charge_window() override { opens++; }

    void close_charge_window() override { closes++; }
};

/** Same machine, with a clock the test winds forward by hand. */
class TestPowerFsm : public PowerFsm
{
public:
    using PowerFsm::PowerFsm;

    int64_t clock = 0;

    void advance(int64_t ms) { clock += ms; }

protected:
    int64_t now_ms() const override { return clock; }
};

/**
 * Tick until the state stops moving.
 *
 * The real loop does the same after a sample: one reading taken on a flat cell
 * walks UNKNOWN, NORMAL, LOW, CRITICAL, and stopping halfway would leave the
 * device believing a flat cell is a healthy one.
 */
void settle(TestPowerFsm &fsm)
{
    for (int i = 0; i < 16; i++) {
        const int before = fsm.get_current_state();

        fsm.run();

        if (fsm.get_current_state() == before) {
            return;
        }
    }

    zassert_unreachable("state machine did not settle");
}

/** Feed one reading and let the ladder finish with it. */
void sample(TestPowerFsm &fsm, uint16_t mv, bool usb)
{
    fsm.post_sample(mv, usb);
    settle(fsm);
}

/** A healthy cell, on battery, settled. */
void boot_to_normal(TestPowerFsm &fsm)
{
    sample(fsm, 3800, false);
    zassert_equal(fsm.get_current_state(), STATE(NORMAL));
}

} // namespace

ZTEST_SUITE(tk_power, NULL, NULL, NULL, NULL, NULL);

ZTEST(tk_power, test_nothing_is_claimed_before_the_first_reading)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);

    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(UNKNOWN));
    zassert_equal(fsm.millivolts(), 0);
}

ZTEST(tk_power, test_an_unknown_cell_still_lets_the_panel_refresh)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);

    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(UNKNOWN));
    zassert_true(fsm.refresh_allowed(), "a dead ADC must not stop the device working");
}

ZTEST(tk_power, test_one_reading_on_a_flat_cell_walks_the_whole_ladder)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);

    // Below the critical threshold, from a standing start, with no second
    // sample to follow it up.
    sample(fsm, 2900, false);

    zassert_equal(fsm.get_current_state(), STATE(CRITICAL));
    zassert_false(fsm.refresh_allowed());
}

ZTEST(tk_power, test_the_refresh_floor_is_where_low_begins)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);

    boot_to_normal(fsm);
    zassert_true(fsm.refresh_allowed());

    sample(fsm, kRefreshMinMv, false);
    zassert_equal(fsm.get_current_state(), STATE(NORMAL), "the floor itself is still normal");

    sample(fsm, kRefreshMinMv - 1, false);
    zassert_equal(fsm.get_current_state(), STATE(LOW));
    zassert_false(fsm.refresh_allowed());
}

ZTEST(tk_power, test_a_cell_that_sags_and_recovers_does_not_flap)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);

    boot_to_normal(fsm);

    sample(fsm, kRefreshMinMv - 50, false);
    zassert_equal(fsm.get_current_state(), STATE(LOW));

    // Back over the floor, but not clear of it. A refresh sags the pack by
    // hundreds of millivolts, so recovering to exactly the threshold means
    // nothing.
    sample(fsm, kRefreshMinMv + 10, false);
    zassert_equal(fsm.get_current_state(), STATE(LOW), "inside the hysteresis band");

    sample(fsm, kRefreshMinMv + kHysteresisMv, false);
    zassert_equal(fsm.get_current_state(), STATE(NORMAL));
}

ZTEST(tk_power, test_getting_worse_is_immediate_and_getting_better_is_not)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);

    boot_to_normal(fsm);

    sample(fsm, kCriticalMv - 1, false);
    zassert_equal(fsm.get_current_state(), STATE(CRITICAL), "no hysteresis on the way down");

    sample(fsm, kCriticalMv + 1, false);
    zassert_equal(fsm.get_current_state(), STATE(CRITICAL), "but plenty on the way up");

    sample(fsm, kCriticalMv + kHysteresisMv, false);
    zassert_equal(fsm.get_current_state(), STATE(LOW));
    zassert_false(fsm.refresh_allowed(), "still under the refresh floor");
}

ZTEST(tk_power, test_plugging_in_from_anywhere_reaches_charging)
{
    const uint16_t from[] = {3800, 3100, 2900};

    for (size_t i = 0; i < ARRAY_SIZE(from); i++) {
        FakePowerIo io;
        TestPowerFsm fsm(io);

        sample(fsm, from[i], false);
        sample(fsm, from[i], true);

        zassert_equal(fsm.get_current_state(), STATE(CHARGING), "from %u mV", from[i]);
        zassert_true(fsm.refresh_allowed(), "external power pays for the refresh");
        zassert_true(fsm.external());
    }
}

ZTEST(tk_power, test_a_full_cell_on_the_charger_reads_charged)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);

    sample(fsm, 3900, true);
    zassert_equal(fsm.get_current_state(), STATE(CHARGING));

    sample(fsm, kFullMv, true);
    zassert_equal(fsm.get_current_state(), STATE(CHARGED));

    // Sitting on the threshold under a live load must not oscillate.
    sample(fsm, kFullMv - 10, true);
    zassert_equal(fsm.get_current_state(), STATE(CHARGED), "inside the hysteresis band");

    sample(fsm, kFullMv - kHysteresisMv, true);
    zassert_equal(fsm.get_current_state(), STATE(CHARGING));
}

ZTEST(tk_power, test_unplugging_forgets_what_it_thought_it_knew)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);

    sample(fsm, kFullMv + 20, true);
    zassert_equal(fsm.get_current_state(), STATE(CHARGED));

    // Surface charge makes the first reading after a charge a lie, so the
    // machine re-derives rather than trusting where it just was.
    fsm.post_sample(kFullMv + 20, false);
    fsm.run();
    zassert_equal(fsm.get_current_state(), STATE(UNKNOWN));

    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(NORMAL));

    sample(fsm, 2950, false);
    zassert_equal(fsm.get_current_state(), STATE(CRITICAL));
}

ZTEST(tk_power, test_the_charge_window_opens_once_per_plug_in)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);

    sample(fsm, 3900, true);
    zassert_equal(io.opens, 1);
    zassert_true(fsm.charge_window_open());

    // Across the full threshold and back. Each crossing is a real state
    // change, and none of them is a new plug-in.
    sample(fsm, kFullMv + 10, true);
    sample(fsm, kFullMv - kHysteresisMv, true);
    sample(fsm, kFullMv + 10, true);

    zassert_equal(io.opens, 1, "a taper wandering across the threshold is not three charges");
    zassert_equal(io.closes, 0);
    zassert_true(fsm.charge_window_open());
}

ZTEST(tk_power, test_the_window_closes_on_the_clock_and_stays_closed)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);

    sample(fsm, 3900, true);
    zassert_true(fsm.charge_window_open());

    fsm.advance(kChargeWindowMs - 1);
    fsm.run();
    zassert_true(fsm.charge_window_open());
    zassert_equal(io.closes, 0);

    fsm.advance(1);
    fsm.run();
    zassert_false(fsm.charge_window_open());
    zassert_equal(io.closes, 1);

    // The device is still awake and still plugged in, so this keeps ticking.
    fsm.advance(kChargeWindowMs);
    fsm.run();
    fsm.run();
    zassert_equal(io.closes, 1, "closing is not something to keep doing");
    zassert_true(fsm.external(), "still on external power, window or no window");
}

ZTEST(tk_power, test_a_spent_window_is_not_reopened_by_reaching_charged)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);

    sample(fsm, 3900, true);
    fsm.advance(kChargeWindowMs);
    fsm.run();
    zassert_equal(io.closes, 1);

    sample(fsm, kFullMv + 10, true);

    zassert_equal(fsm.get_current_state(), STATE(CHARGED));
    zassert_equal(io.opens, 1, "the same plug-in does not buy a second window");
    zassert_false(fsm.charge_window_open());
}

ZTEST(tk_power, test_unplugging_closes_the_window_early_and_only_once)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);

    sample(fsm, 3900, true);
    zassert_true(fsm.charge_window_open());

    fsm.advance(1000);
    sample(fsm, 3900, false);

    zassert_false(fsm.charge_window_open());
    zassert_equal(io.closes, 1);
    zassert_false(fsm.external());

    // Whatever happens next, the window that was closed stays closed.
    fsm.advance(kChargeWindowMs);
    fsm.run();
    zassert_equal(io.closes, 1);
}

ZTEST(tk_power, test_a_second_plug_in_gets_its_own_window)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);

    sample(fsm, 3900, true);
    sample(fsm, 3900, false);
    zassert_equal(io.opens, 1);
    zassert_equal(io.closes, 1);

    sample(fsm, 3900, true);

    zassert_equal(io.opens, 2);
    zassert_true(fsm.charge_window_open());
}

ZTEST(tk_power, test_every_state_change_is_published)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);

    settle(fsm);
    const int at_boot = io.publishes;
    zassert_true(at_boot >= 1, "the channel carries a value before anything reads it");
    zassert_equal(io.last_state, PowerState::UNKNOWN);

    sample(fsm, 3800, false);
    zassert_equal(io.last_state, PowerState::NORMAL);
    zassert_equal(io.last_mv, 3800);
    zassert_false(io.last_usb);

    sample(fsm, 3800, true);
    zassert_equal(io.last_state, PowerState::CHARGING);
    zassert_true(io.last_usb);
}

ZTEST(tk_power, test_a_reading_that_has_barely_moved_is_not_published)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);

    boot_to_normal(fsm);
    const int settled = io.publishes;

    sample(fsm, 3800 - (kPublishDeadbandMv - 1), false);
    zassert_equal(io.publishes, settled, "ADC noise is not news");

    sample(fsm, 3800 - kPublishDeadbandMv, false);
    zassert_equal(io.publishes, settled + 1, "a real drop is");
}
