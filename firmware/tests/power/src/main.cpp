
#include <zephyr/ztest.h>

#include "power_fsm.hpp"

using namespace cicala;
using State = PowerFsm::State;

namespace
{

class FakePowerIo : public PowerIo
{
public:
    int publishes = 0;
    PowerState last_state = PowerState::UNKNOWN;
    uint16_t last_mv = 0;
    bool last_usb = false;
    ChargerStatus last_charger = ChargerStatus::NOT_MONITORED;

    int opens = 0;
    int closes = 0;

    void publish(PowerState state, uint16_t mv, bool usb, ChargerStatus charger) override
    {
        publishes++;
        last_state = state;
        last_mv = mv;
        last_usb = usb;
        last_charger = charger;
    }

    void open_charge_window() override { opens++; }

    void close_charge_window() override { closes++; }
};

class TestPowerFsm : public PowerFsm
{
public:
    using PowerFsm::PowerFsm;

    int64_t clock = 0;

    void advance(int64_t ms) { clock += ms; }

protected:
    int64_t now_ms() const override { return clock; }
};

void settle(TestPowerFsm &fsm)
{
    /* One reading may cross several voltage thresholds. */
    for (int i = 0; i < 16; i++) {
        const int before = fsm.get_current_state();

        fsm.run();

        if (fsm.get_current_state() == before) {
            return;
        }
    }

    zassert_unreachable("state machine did not settle");
}

void sample(TestPowerFsm &fsm, uint16_t mv, bool usb)
{
    fsm.post_sample(mv, usb);
    settle(fsm);
}

void boot_to_normal(TestPowerFsm &fsm)
{
    sample(fsm, 3800, false);
    zassert_equal(fsm.get_current_state(), STATE(NORMAL));
}

} // namespace

ZTEST_SUITE(cicala_power, NULL, NULL, NULL, NULL, NULL);

ZTEST(cicala_power, test_nothing_is_claimed_before_the_first_reading)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);

    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(UNKNOWN));
    zassert_equal(fsm.millivolts(), 0);
}

ZTEST(cicala_power, test_an_unknown_cell_still_lets_the_panel_refresh)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);

    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(UNKNOWN));
    zassert_true(fsm.refresh_allowed(), "a dead ADC must not stop the device working");
}

ZTEST(cicala_power, test_one_reading_on_a_flat_cell_walks_the_whole_ladder)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);

    sample(fsm, 2900, false);

    zassert_equal(fsm.get_current_state(), STATE(CRITICAL));
    zassert_false(fsm.refresh_allowed());
}

ZTEST(cicala_power, test_a_reading_too_low_to_be_a_cell_is_not_a_flat_cell)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);

    sample(fsm, kPlausibleMv - 1, false);

    zassert_equal(fsm.get_current_state(), STATE(UNKNOWN));
    zassert_true(fsm.refresh_allowed(), "an implausible reading must not blank the panel");
}

ZTEST(cicala_power, test_the_floor_itself_is_a_cell)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);

    sample(fsm, kPlausibleMv, false);

    zassert_equal(fsm.get_current_state(), STATE(CRITICAL));
    zassert_false(fsm.refresh_allowed());
}

ZTEST(cicala_power, test_a_cell_that_falls_off_the_bottom_gives_the_panel_back)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);

    sample(fsm, kCriticalMv - 1, false);
    zassert_equal(fsm.get_current_state(), STATE(CRITICAL));

    sample(fsm, kPlausibleMv - 1, false);
    zassert_equal(fsm.get_current_state(), STATE(UNKNOWN));
    zassert_true(fsm.refresh_allowed());

    sample(fsm, 3800, false);
    zassert_equal(fsm.get_current_state(), STATE(NORMAL), "and a real reading is believed again");
}

ZTEST(cicala_power, test_the_floor_leaves_the_hysteresis_alone)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);

    boot_to_normal(fsm);

    sample(fsm, kRefreshMinMv - 50, false);
    zassert_equal(fsm.get_current_state(), STATE(LOW));

    sample(fsm, kRefreshMinMv + 10, false);
    zassert_equal(fsm.get_current_state(), STATE(LOW), "still inside the hysteresis band");

    sample(fsm, kRefreshMinMv + kHysteresisMv, false);
    zassert_equal(fsm.get_current_state(), STATE(NORMAL));
}

ZTEST(cicala_power, test_external_power_outranks_the_floor)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);

    sample(fsm, kPlausibleMv - 1, true);

    zassert_equal(fsm.get_current_state(), STATE(CHARGING));
    zassert_true(fsm.external());
    zassert_equal(io.opens, 1);
}

ZTEST(cicala_power, test_the_refresh_floor_is_where_low_begins)
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

ZTEST(cicala_power, test_a_cell_that_sags_and_recovers_does_not_flap)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);

    boot_to_normal(fsm);

    sample(fsm, kRefreshMinMv - 50, false);
    zassert_equal(fsm.get_current_state(), STATE(LOW));

    sample(fsm, kRefreshMinMv + 10, false);
    zassert_equal(fsm.get_current_state(), STATE(LOW), "inside the hysteresis band");

    sample(fsm, kRefreshMinMv + kHysteresisMv, false);
    zassert_equal(fsm.get_current_state(), STATE(NORMAL));
}

ZTEST(cicala_power, test_getting_worse_is_immediate_and_getting_better_is_not)
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

ZTEST(cicala_power, test_plugging_in_from_anywhere_reaches_charging)
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

ZTEST(cicala_power, test_a_full_cell_on_the_charger_reads_charged)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);

    sample(fsm, 3900, true);
    zassert_equal(fsm.get_current_state(), STATE(CHARGING));

    sample(fsm, kFullMv, true);
    zassert_equal(fsm.get_current_state(), STATE(CHARGED));

    sample(fsm, kFullMv - 10, true);
    zassert_equal(fsm.get_current_state(), STATE(CHARGED), "inside the hysteresis band");

    sample(fsm, kFullMv - kHysteresisMv, true);
    zassert_equal(fsm.get_current_state(), STATE(CHARGING));
}

ZTEST(cicala_power, test_unplugging_forgets_what_it_thought_it_knew)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);

    sample(fsm, kFullMv + 20, true);
    zassert_equal(fsm.get_current_state(), STATE(CHARGED));

    fsm.post_sample(kFullMv + 20, false);
    fsm.run();
    zassert_equal(fsm.get_current_state(), STATE(UNKNOWN));

    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(NORMAL));

    sample(fsm, 2950, false);
    zassert_equal(fsm.get_current_state(), STATE(CRITICAL));
}

ZTEST(cicala_power, test_the_charge_window_opens_once_per_plug_in)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);

    sample(fsm, 3900, true);
    zassert_equal(io.opens, 1);
    zassert_true(fsm.charge_window_open());

    sample(fsm, kFullMv + 10, true);
    sample(fsm, kFullMv - kHysteresisMv, true);
    sample(fsm, kFullMv + 10, true);

    zassert_equal(io.opens, 1, "a taper wandering across the threshold is not three charges");
    zassert_equal(io.closes, 0);
    zassert_true(fsm.charge_window_open());
}

ZTEST(cicala_power, test_the_window_closes_on_the_clock_and_stays_closed)
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

    fsm.advance(kChargeWindowMs);
    fsm.run();
    fsm.run();
    zassert_equal(io.closes, 1, "closing is not something to keep doing");
    zassert_true(fsm.external(), "still on external power, window or no window");
}

ZTEST(cicala_power, test_a_spent_window_is_not_reopened_by_reaching_charged)
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

ZTEST(cicala_power, test_unplugging_closes_the_window_early_and_only_once)
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

    fsm.advance(kChargeWindowMs);
    fsm.run();
    zassert_equal(io.closes, 1);
}

ZTEST(cicala_power, test_a_second_plug_in_gets_its_own_window)
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

ZTEST(cicala_power, test_the_usb_pin_is_believed_without_a_reading)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);

    fsm.post_usb(true);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(CHARGING));
    zassert_true(fsm.external(), "net asks this before it joins a network");
    zassert_equal(io.opens, 1, "and the sync window is the reason it asks");
}

ZTEST(cicala_power, test_unplugging_is_noticed_after_the_adc_stops_answering)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);

    sample(fsm, 3900, true);
    zassert_equal(fsm.get_current_state(), STATE(CHARGING));

    fsm.post_usb(false);
    settle(fsm);

    zassert_false(fsm.external(), "sleep_now() would otherwise never sleep again");
    zassert_equal(io.closes, 1, "and the charge window would never close");
}

ZTEST(cicala_power, test_the_usb_pin_alone_is_not_a_measurement)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);

    fsm.post_usb(false);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(UNKNOWN));
    zassert_equal(fsm.millivolts(), 0);
    zassert_true(fsm.refresh_allowed());
}

ZTEST(cicala_power, test_every_state_change_is_published)
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

ZTEST(cicala_power, test_a_reading_that_has_barely_moved_is_not_published)
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

ZTEST(cicala_power, test_charger_status_overrides_voltage_estimate)
{
    const ChargerStatus statuses[] = {
        ChargerStatus::CHARGING,      ChargerStatus::IDLE,        ChargerStatus::RECOVERABLE_FAULT,
        ChargerStatus::LATCHED_FAULT, ChargerStatus::UNAVAILABLE,
    };
    const State states[] = {State::CHARGING, State::EXTERNAL_IDLE, State::CHARGE_FAULT,
                            State::CHARGE_FAULT, State::EXTERNAL_IDLE};
    const uint16_t voltages[] = {2900, 3100, 3900, 4200};
    for (uint16_t voltage : voltages) {
        for (size_t i = 0; i < ARRAY_SIZE(statuses); i++) {
            FakePowerIo io;
            TestPowerFsm fsm(io);
            sample(fsm, voltage, false);
            fsm.post_charger(statuses[i]);
            sample(fsm, voltage, true);
            zassert_equal(fsm.state(), states[i]);
            zassert_equal(io.last_charger, statuses[i]);
            zassert_true(fsm.refresh_allowed());
            zassert_equal(io.opens, 1);
        }
    }
}

ZTEST(cicala_power, test_charger_idle_never_proves_a_full_cell)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);
    fsm.post_charger(ChargerStatus::CHARGING);
    sample(fsm, 4200, true);
    zassert_equal(fsm.state(), State::CHARGING, "CV charging can continue at 4.2 V");
    fsm.post_charger(ChargerStatus::IDLE);
    settle(fsm);
    zassert_equal(fsm.state(), State::EXTERNAL_IDLE, "high/high may be a thermal CE veto");
    fsm.post_charger(ChargerStatus::CHARGING);
    settle(fsm);
    zassert_equal(fsm.state(), State::CHARGING);
}

ZTEST(cicala_power, test_charger_faults_do_not_reopen_the_usb_window)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);
    fsm.post_charger(ChargerStatus::CHARGING);
    sample(fsm, 3900, true);
    const ChargerStatus statuses[] = {ChargerStatus::RECOVERABLE_FAULT, ChargerStatus::IDLE,
                                      ChargerStatus::LATCHED_FAULT, ChargerStatus::CHARGING};
    for (ChargerStatus status : statuses) {
        fsm.post_charger(status);
        fsm.advance(kChargeWindowMs);
        settle(fsm);
        zassert_true(fsm.external());
        zassert_false(fsm.charge_window_open());
    }
    zassert_equal(io.opens, 1);
    zassert_equal(io.closes, 1);
}

ZTEST(cicala_power, test_fault_detail_publishes_without_a_voltage_change)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);
    fsm.post_charger(ChargerStatus::RECOVERABLE_FAULT);
    sample(fsm, 3900, true);
    const int before = io.publishes;
    fsm.post_charger(ChargerStatus::LATCHED_FAULT);
    settle(fsm);
    zassert_equal(fsm.state(), State::CHARGE_FAULT);
    zassert_equal(io.publishes, before + 1);
    zassert_equal(io.last_charger, ChargerStatus::LATCHED_FAULT);
}

ZTEST(cicala_power, test_unplugging_during_a_fault_rechecks_the_cell)
{
    FakePowerIo io;
    TestPowerFsm fsm(io);
    fsm.post_charger(ChargerStatus::RECOVERABLE_FAULT);
    sample(fsm, 2900, true);
    sample(fsm, 2900, false);
    zassert_equal(fsm.state(), State::CRITICAL);
    zassert_false(fsm.refresh_allowed());
    zassert_false(fsm.external());
}
