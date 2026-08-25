
#include <zephyr/ztest.h>

#include "status_led.hpp"

using namespace cicala;

namespace
{

constexpr int64_t kBlinkPair = 2 * kStatusBlinkMs;

constexpr int64_t after(uint8_t blinks)
{
    return (int64_t) blinks * kBlinkPair;
}

} // namespace

ZTEST_SUITE(cicala_status, NULL, NULL, NULL, NULL, NULL);

ZTEST(cicala_status, test_a_healthy_cell_with_nothing_happening_is_dark)
{
    StatusLed led;

    led.set_power(PowerState::NORMAL, false);

    const Pattern shown = led.pattern(0);

    zassert_equal(shown.colour, Colour::OFF);
    zassert_equal(shown.rhythm, Rhythm::STEADY);
    zassert_false(led.animating(0));
}

ZTEST(cicala_status, test_charging_is_red_and_charged_is_green)
{
    StatusLed led;

    led.set_power(PowerState::CHARGING, false);
    zassert_equal(led.pattern(0).colour, Colour::RED);
    zassert_equal(led.pattern(0).rhythm, Rhythm::STEADY);

    led.set_power(PowerState::CHARGED, false);
    zassert_equal(led.pattern(0).colour, Colour::GREEN);
    zassert_equal(led.pattern(0).rhythm, Rhythm::STEADY);
}

ZTEST(cicala_status, test_the_portal_outranks_a_charged_cell)
{
    StatusLed led;

    led.set_power(PowerState::CHARGED, false);
    led.set_portal(true);

    zassert_equal(led.pattern(0).colour, Colour::AMBER, "setup is what somebody is doing");

    led.set_portal(false);
    zassert_equal(led.pattern(0).colour, Colour::GREEN, "and it hands back when it ends");
}

ZTEST(cicala_status, test_a_transfer_outranks_the_portal_and_hands_back)
{
    StatusLed led;

    led.set_power(PowerState::CHARGING, false);
    led.set_portal(true);
    zassert_equal(led.pattern(0).colour, Colour::AMBER);

    led.set_activity(true);
    const Pattern busy = led.pattern(0);
    zassert_equal(busy.colour, Colour::GREEN);
    zassert_equal(busy.rhythm, Rhythm::PULSE);

    led.set_activity(false);
    zassert_equal(led.pattern(0).colour, Colour::AMBER);

    led.set_portal(false);
    zassert_equal(led.pattern(0).colour, Colour::RED, "back to the charge underneath it all");
}

ZTEST(cicala_status, test_a_blocked_press_on_a_low_cell_blinks_amber_once)
{
    StatusLed led;

    led.set_power(PowerState::LOW, false);

    const Pattern first = led.pattern(0);
    zassert_equal(first.colour, Colour::AMBER);
    zassert_equal(first.rhythm, Rhythm::BLINK);
    zassert_equal(first.count, 1);

    zassert_equal(led.pattern(after(1)).rhythm, Rhythm::STEADY, "one pair and no more");
}

ZTEST(cicala_status, test_a_blocked_press_on_a_flat_cell_blinks_red_three_times)
{
    StatusLed led;

    led.set_power(PowerState::CRITICAL, false);

    zassert_equal(led.pattern(0).colour, Colour::RED);
    zassert_equal(led.pattern(0).count, 3);
    zassert_equal(led.pattern(kBlinkPair).count, 2);
    zassert_equal(led.pattern(2 * kBlinkPair).count, 1);

    const Pattern done = led.pattern(after(3));
    zassert_equal(done.rhythm, Rhythm::STEADY);
    zassert_equal(done.colour, Colour::OFF, "a flat cell is not worth lighting for");
}

ZTEST(cicala_status, test_pressing_again_asks_again_and_is_answered_again)
{
    StatusLed led;

    led.set_power(PowerState::CRITICAL, false);
    zassert_equal(led.pattern(0).count, 3);

    const int64_t later = after(3) + 5000;
    zassert_equal(led.pattern(later).rhythm, Rhythm::STEADY);

    led.set_power(PowerState::CRITICAL, true);

    const Pattern again = led.pattern(later);
    zassert_equal(again.rhythm, Rhythm::BLINK);
    zassert_equal(again.count, 3, "the second press deserves the same answer as the first");
}

ZTEST(cicala_status, test_a_steady_colour_does_not_blink_at_a_passer_by)
{
    StatusLed led;

    led.set_power(PowerState::CHARGING, false);

    for (int64_t t = 0; t < 10000; t += 137) {
        zassert_equal(led.pattern(t).rhythm, Rhythm::STEADY, "at %lld ms", (long long) t);
        zassert_equal(led.output(t), Colour::RED, "at %lld ms", (long long) t);
    }
}

ZTEST(cicala_status, test_a_blink_and_a_steady_colour_are_told_apart_by_more_than_the_eye)
{
    StatusLed blinking;
    StatusLed steady;

    blinking.set_power(PowerState::LOW, false);
    steady.set_portal(true);

    zassert_equal(blinking.pattern(0).colour, steady.pattern(0).colour);
    zassert_not_equal(blinking.pattern(0).rhythm, steady.pattern(0).rhythm);
}

ZTEST(cicala_status, test_a_blink_is_actually_off_half_the_time)
{
    StatusLed led;

    led.set_power(PowerState::CRITICAL, false);

    zassert_equal(led.output(0), Colour::RED);
    zassert_equal(led.output(kStatusBlinkMs), Colour::OFF);
    zassert_equal(led.output(2 * kStatusBlinkMs), Colour::RED);
    zassert_equal(led.output(3 * kStatusBlinkMs), Colour::OFF);

    zassert_equal(led.output(after(3)), Colour::OFF, "and dark once the burst has run");
}

ZTEST(cicala_status, test_a_pulse_is_on_for_half_its_cycle)
{
    StatusLed led;

    led.set_power(PowerState::CHARGING, false);
    led.set_activity(true);

    zassert_equal(led.output(0), Colour::GREEN);
    zassert_equal(led.output(kStatusPulseMs / 2), Colour::OFF);
    zassert_equal(led.output(kStatusPulseMs), Colour::GREEN);
}

ZTEST(cicala_status, test_only_something_moving_asks_to_be_looked_at_again)
{
    StatusLed led;

    led.set_power(PowerState::CHARGED, false);
    zassert_false(led.animating(0), "a steady colour needs no work queue");

    led.set_activity(true);
    zassert_true(led.animating(0));

    led.set_activity(false);
    led.set_power(PowerState::CRITICAL, false);
    zassert_true(led.animating(0));
    zassert_false(led.animating(after(3)), "and stops asking once the burst has run");
}
