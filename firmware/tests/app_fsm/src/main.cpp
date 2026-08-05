/*
 * The tabletop state machine. No board, no display, no channels: the fake Io
 * below counts what was asked for and the clock is injected, so the refresh
 * timeout is tested without waiting for it.
 *
 * The shape to keep in mind: turning the selector announces the deck's name
 * and leaves it up, and Next is what asks for a question. So a deck change and
 * a press do different things, and the machine has a state for each.
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
    int labels = 0;
    int last_deck = -1;
    int last_labelled_deck = -1;
    bool draw_succeeds = true;
    bool label_succeeds = true;
    bool retained = false;
    int retained_deck = -1;

    bool draw(uint8_t deck) override
    {
        draws++;
        last_deck = deck;
        return draw_succeeds;
    }

    bool show_category(uint8_t deck) override
    {
        labels++;
        last_labelled_deck = deck;
        return label_succeeds;
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
 * The real loop does the same between zbus messages: one event can walk the
 * machine through several states — a press is SHOWING, DRAWING, REFRESHING —
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

/** Boot with `deck` selected, through the deck name, to the steady state. */
void boot_to_showing(TestAppFsm &fsm, FakeIo &io, uint8_t deck)
{
    fsm.post_selector(deck, true);
    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(REFRESHING));

    fsm.post_render(true);
    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(SHOWING));
    zassert_equal(io.labels, 1, "boot announces the deck");
    zassert_equal(io.draws, 0, "and does not ask a question yet");
}

/** Boot, then press Next so a question is what is on the panel. */
void boot_to_question(TestAppFsm &fsm, FakeIo &io, uint8_t deck)
{
    boot_to_showing(fsm, io, deck);

    fsm.post_next();
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
    zassert_equal(io.labels, 0, "nor name one");
}

ZTEST(tk_app_fsm, test_boot_announces_the_deck)
{
    FakeIo io;
    TestAppFsm fsm(io);

    fsm.post_selector(4, true);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(REFRESHING));
    zassert_equal(io.labels, 1);
    zassert_equal(io.last_labelled_deck, 4);
    zassert_equal(io.draws, 0, "the first question waits for a press");
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
    zassert_equal(io.labels, 0);
}

/*
 * Waking by Next. The press that ended the sleep is spent on the wake itself,
 * so app_logic replays it into the machine before BOOT runs; from here that is
 * indistinguishable from a press that was already pending.
 */
ZTEST(tk_app_fsm, test_a_next_pending_at_boot_is_answered_with_a_question)
{
    FakeIo io;
    io.retained = true;
    io.retained_deck = 2;

    TestAppFsm fsm(io);

    fsm.post_next();
    fsm.post_selector(2, true);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(REFRESHING));
    zassert_equal(io.draws, 1, "waking by Next must answer, not sit on the old question");
    zassert_equal(io.labels, 0, "and must not announce the deck first");
    zassert_equal(io.last_deck, 2);
}

/*
 * The same, with nothing retained: a Next wake onto a panel showing a deck
 * name. Announcing first would cost a refresh, and REFRESHING drops presses
 * made during one — so the press would be swallowed and the table would see a
 * button that does nothing.
 */
ZTEST(tk_app_fsm, test_a_next_pending_at_boot_beats_the_deck_announcement)
{
    FakeIo io;
    TestAppFsm fsm(io);

    fsm.post_next();
    fsm.post_selector(3, true);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(REFRESHING));
    zassert_equal(io.draws, 1);
    zassert_equal(io.labels, 0);
}

/** And having answered it, the machine settles rather than relabelling. */
ZTEST(tk_app_fsm, test_a_next_wake_settles_without_announcing_the_deck)
{
    FakeIo io;
    io.retained = true;
    io.retained_deck = 1;

    TestAppFsm fsm(io);

    fsm.post_next();
    fsm.post_selector(1, true);
    settle(fsm);

    fsm.post_render(true);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(SHOWING));
    zassert_equal(io.draws, 1);
    zassert_equal(io.labels, 0, "the deck was never in doubt; naming it would be noise");
}

ZTEST(tk_app_fsm, test_next_asks_for_a_question)
{
    FakeIo io;
    TestAppFsm fsm(io);

    boot_to_showing(fsm, io, 1);

    fsm.post_next();
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(REFRESHING));
    zassert_equal(io.draws, 1);
    zassert_equal(io.last_deck, 1);
    zassert_equal(io.labels, 1, "and does not repeat the deck name");
}

ZTEST(tk_app_fsm, test_next_keeps_asking_from_the_same_deck)
{
    FakeIo io;
    TestAppFsm fsm(io);

    boot_to_question(fsm, io, 3);

    fsm.post_next();
    settle(fsm);

    zassert_equal(io.draws, 2);
    zassert_equal(io.last_deck, 3);
    zassert_equal(io.labels, 1, "the name is announced once, when the deck changes");
}

ZTEST(tk_app_fsm, test_turning_the_selector_announces_the_new_deck)
{
    FakeIo io;
    TestAppFsm fsm(io);

    boot_to_question(fsm, io, 1);

    fsm.post_selector(3, true);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(REFRESHING));
    zassert_equal(io.labels, 2);
    zassert_equal(io.last_labelled_deck, 3);
    zassert_equal(io.draws, 1, "turning the knob is not a request for a question");
}

ZTEST(tk_app_fsm, test_the_deck_name_stays_until_next)
{
    FakeIo io;
    TestAppFsm fsm(io);

    boot_to_question(fsm, io, 1);

    fsm.post_selector(5, true);
    settle(fsm);
    fsm.post_render(true);
    settle(fsm);

    // Sitting there does not turn the name into a question.
    for (int i = 0; i < 5; i++) {
        settle(fsm);
    }

    zassert_equal(fsm.get_current_state(), STATE(SHOWING));
    zassert_equal(io.draws, 1, "the name stays up until someone asks");

    fsm.post_next();
    settle(fsm);

    zassert_equal(io.draws, 2);
    zassert_equal(io.last_deck, 5, "and the question comes from the deck just named");
}

ZTEST(tk_app_fsm, test_an_invalid_selector_keeps_what_is_on_the_panel)
{
    FakeIo io;
    TestAppFsm fsm(io);

    boot_to_question(fsm, io, 1);

    fsm.post_selector(0, false);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(SHOWING));
    zassert_equal(io.draws, 1, "an invalid selector is not a deck change");
    zassert_equal(io.labels, 1);
}

ZTEST(tk_app_fsm, test_a_press_during_a_refresh_is_dropped)
{
    FakeIo io;
    TestAppFsm fsm(io);

    boot_to_question(fsm, io, 1);

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
    zassert_equal(io.draws, 1);

    fsm.post_render(true);
    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(SHOWING));
    zassert_equal(io.draws, 1, "and exactly one question was drawn for that press");
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

    boot_to_showing(fsm, io, 5);

    fsm.post_next();
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(SHOWING));
    zassert_equal(io.draws, 1, "a deck that yields nothing must not be retried in a loop");
}

ZTEST(tk_app_fsm, test_an_empty_deck_is_retried_on_the_next_press)
{
    FakeIo io;
    io.draw_succeeds = false;

    TestAppFsm fsm(io);

    boot_to_showing(fsm, io, 5);

    fsm.post_next();
    settle(fsm);
    zassert_equal(io.draws, 1);

    io.draw_succeeds = true;
    fsm.post_next();
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(REFRESHING));
    zassert_equal(io.draws, 2);
}

ZTEST(tk_app_fsm, test_a_deck_name_that_cannot_be_shown_does_not_loop)
{
    FakeIo io;
    io.label_succeeds = false;

    TestAppFsm fsm(io);

    fsm.post_selector(2, true);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(SHOWING),
                  "it should fail through to the steady state");
    zassert_equal(io.labels, 1, "and not keep trying to announce the same deck");
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
