/*
 * The tabletop state machine. No board, no display, no channels: the fake Io
 * below counts draws and the clock is injected, so the five-second refresh
 * timeout is tested without waiting five seconds.
 */

#include <zephyr/ztest.h>

#include "app_fsm.hpp"

using namespace tk;
using State = AppFsm::State;

namespace
{

/** Records what the state machine asked for, and answers how the test says to. */
class FakeIo : public AppIo
{
public:
    int draws = 0;
    int last_deck = -1;
    bool draw_succeeds = true;
    bool retained = false;
    int retained_deck = -1;

    bool draw(uint8_t deck) override
    {
        draws++;
        last_deck = deck;
        return draw_succeeds;
    }

    bool retained_matches(uint8_t deck) const override
    {
        return retained && retained_deck == (int) deck;
    }
};

/** Same machine, with a clock the test winds forward by hand. */
class TestAppFsm : public AppFsm
{
public:
    using AppFsm::AppFsm;

    int64_t clock = 0;

    void advance(int64_t ms) { clock += ms; }

protected:
    int64_t now_ms() const override { return clock; }
};

/**
 * Tick until the state stops moving.
 *
 * The real loop does the same thing between zbus messages: one event can walk
 * the machine through several states (a press is SHOWING, DRAWING, REFRESHING)
 * and it must not stop halfway.
 */
void settle(TestAppFsm &fsm)
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

/** Boot to the steady state with `deck` selected and a question drawn. */
void boot_to_showing(TestAppFsm &fsm, FakeIo &io, uint8_t deck)
{
    fsm.post_selector(deck, true);
    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(REFRESHING));

    fsm.post_render(true);
    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(SHOWING));
    zassert_equal(io.last_deck, deck);
}

} // namespace

ZTEST_SUITE(tk_app_fsm, NULL, NULL, NULL, NULL, NULL);

ZTEST(tk_app_fsm, test_boot_waits_for_a_valid_selector)
{
    FakeIo io;
    TestAppFsm fsm(io);

    fsm.post_selector(0, false);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(BOOT));
    zassert_equal(io.draws, 0, "a mid-travel selector must not pick a deck");
}

ZTEST(tk_app_fsm, test_boot_with_nothing_retained_draws)
{
    FakeIo io;
    TestAppFsm fsm(io);

    fsm.post_selector(4, true);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(REFRESHING));
    zassert_equal(io.draws, 1);
    zassert_equal(io.last_deck, 4);
}

ZTEST(tk_app_fsm, test_boot_with_a_retained_question_draws_nothing)
{
    FakeIo io;
    io.retained = true;
    io.retained_deck = 2;

    TestAppFsm fsm(io);

    fsm.post_selector(2, true);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(SHOWING));
    zassert_equal(io.draws, 0, "e-paper kept the question; waking must be free");
}

ZTEST(tk_app_fsm, test_a_retained_question_from_another_deck_is_not_reused)
{
    FakeIo io;
    io.retained = true;
    io.retained_deck = 2;

    TestAppFsm fsm(io);

    /* Selector moved while the device was asleep. */
    fsm.post_selector(5, true);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(REFRESHING));
    zassert_equal(io.last_deck, 5);
}

ZTEST(tk_app_fsm, test_next_draws_again_from_the_same_deck)
{
    FakeIo io;
    TestAppFsm fsm(io);

    boot_to_showing(fsm, io, 1);

    fsm.post_next();
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(REFRESHING));
    zassert_equal(io.draws, 2);
    zassert_equal(io.last_deck, 1);
}

ZTEST(tk_app_fsm, test_turning_the_selector_draws_from_the_new_deck)
{
    FakeIo io;
    TestAppFsm fsm(io);

    boot_to_showing(fsm, io, 1);

    fsm.post_selector(3, true);
    settle(fsm);

    zassert_equal(io.draws, 2);
    zassert_equal(io.last_deck, 3);
}

ZTEST(tk_app_fsm, test_an_invalid_selector_keeps_the_question)
{
    FakeIo io;
    TestAppFsm fsm(io);

    boot_to_showing(fsm, io, 1);

    fsm.post_selector(0, false);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(SHOWING));
    zassert_equal(io.draws, 1, "an invalid selector is not a deck change");
}

ZTEST(tk_app_fsm, test_a_press_during_a_refresh_is_dropped)
{
    FakeIo io;
    TestAppFsm fsm(io);

    boot_to_showing(fsm, io, 1);

    fsm.post_next();
    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(REFRESHING));
    zassert_equal(io.draws, 2);

    /* Impatient second press, while the panel is still working. */
    fsm.post_next();
    settle(fsm);

    fsm.post_render(true);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(SHOWING));
    zassert_equal(io.draws, 2, "the press made during the refresh must not queue");
}

ZTEST(tk_app_fsm, test_a_panel_that_never_answers_times_out)
{
    FakeIo io;
    TestAppFsm fsm(io);

    fsm.post_selector(0, true);
    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(REFRESHING));

    /* Just short of the timeout, nothing happens. */
    fsm.advance(CONFIG_TK_REFRESH_TIMEOUT_MS - 1);
    fsm.run();
    zassert_equal(fsm.get_current_state(), STATE(REFRESHING));

    fsm.advance(1);
    fsm.run();
    zassert_equal(fsm.get_current_state(), STATE(FAIL), "a dead panel must not wedge the device");

    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(SHOWING));
}

ZTEST(tk_app_fsm, test_a_render_arriving_after_the_timeout_is_not_reused)
{
    /*
     * The bug this pins down, seen on hardware: a slow panel outran the
     * refresh timeout, the machine gave up and went to SHOWING, and the render
     * result landed there with nothing to consume it. The next press then
     * entered REFRESHING and found that stale result waiting, so it left again
     * immediately — the panel was told to draw and the machine called it done
     * in the same breath. One press appeared to do nothing and the next showed
     * two questions in quick succession.
     */
    FakeIo io;
    TestAppFsm fsm(io);

    fsm.post_selector(0, true);
    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(REFRESHING));

    /* The panel takes longer than the machine is willing to wait. */
    fsm.advance(CONFIG_TK_REFRESH_TIMEOUT_MS);
    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(SHOWING));

    /* It finishes anyway, far too late to matter. */
    fsm.post_render(true);
    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(SHOWING), "a late render must not move anything");

    /* The next press has to wait for its own render, not inherit that one. */
    fsm.post_next();
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(REFRESHING),
                  "the machine must still be waiting on the panel it just asked to draw");
    zassert_equal(io.draws, 2);

    fsm.post_render(true);
    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(SHOWING));
    zassert_equal(io.draws, 2, "and exactly one question was drawn for that press");
}

ZTEST(tk_app_fsm, test_a_failed_render_fails_without_waiting)
{
    FakeIo io;
    TestAppFsm fsm(io);

    fsm.post_selector(0, true);
    settle(fsm);

    fsm.post_render(false);
    fsm.run();

    zassert_equal(fsm.get_current_state(), STATE(FAIL));
}

ZTEST(tk_app_fsm, test_an_empty_deck_fails_once_and_settles)
{
    FakeIo io;
    io.draw_succeeds = false;

    TestAppFsm fsm(io);

    fsm.post_selector(5, true);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(SHOWING));
    zassert_equal(io.draws, 1, "a deck that yields nothing must not be retried in a loop");
}

ZTEST(tk_app_fsm, test_an_empty_deck_is_retried_on_the_next_press)
{
    FakeIo io;
    io.draw_succeeds = false;

    TestAppFsm fsm(io);

    fsm.post_selector(5, true);
    settle(fsm);
    zassert_equal(io.draws, 1);

    io.draw_succeeds = true;
    fsm.post_next();
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(REFRESHING));
    zassert_equal(io.draws, 2);
}

ZTEST(tk_app_fsm, test_only_refreshing_asks_the_loop_to_stay_awake)
{
    FakeIo io;
    TestAppFsm fsm(io);

    fsm.post_selector(0, false);
    settle(fsm);
    zassert_false(fsm.current_state_has_timeout(), "BOOT must let the device sleep");

    fsm.post_selector(0, true);
    settle(fsm);
    zassert_true(fsm.current_state_has_timeout(), "REFRESHING has to be watched");

    fsm.post_render(true);
    settle(fsm);
    zassert_false(fsm.current_state_has_timeout(), "SHOWING is where deep sleep happens");
}
