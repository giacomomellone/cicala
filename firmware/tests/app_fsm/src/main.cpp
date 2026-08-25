
#include <zephyr/ztest.h>

#include "app_fsm.hpp"

using namespace cicala;
using State = AppFsm::State;

namespace
{

class FakeIo : public AppIo
{
public:
    int draws = 0;
    int labels = 0;
    int last_deck = -1;
    int last_labelled_deck = -1;
    bool draw_succeeds = true;
    bool label_succeeds = true;
    int services = 0;
    bool service_succeeds = true;
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

    bool show_service() override
    {
        services++;
        return service_succeeds;
    }

    bool retained_matches(uint8_t deck) const override
    {
        return retained && retained_deck == (int) deck;
    }
};

class TestAppFsm : public AppFsm
{
public:
    using AppFsm::AppFsm;

    int64_t clock = 0;

    void advance(int64_t ms) { clock += ms; }

protected:
    int64_t now_ms() const override { return clock; }
};

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

ZTEST_SUITE(cicala_app_fsm, NULL, NULL, NULL, NULL, NULL);

ZTEST(cicala_app_fsm, test_boot_waits_for_a_valid_deck)
{
    FakeIo io;
    TestAppFsm fsm(io);

    fsm.post_selector(0, false);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(BOOT));
    zassert_equal(io.draws, 0, "an invalid deck must not draw");
    zassert_equal(io.labels, 0, "an invalid deck must not be named");
}

ZTEST(cicala_app_fsm, test_boot_announces_the_deck)
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

ZTEST(cicala_app_fsm, test_boot_with_a_retained_question_draws_nothing)
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

ZTEST(cicala_app_fsm, test_a_next_pending_at_boot_is_answered_with_a_question)
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

ZTEST(cicala_app_fsm, test_a_next_pending_at_boot_beats_the_deck_announcement)
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

ZTEST(cicala_app_fsm, test_a_next_wake_settles_without_announcing_the_deck)
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

ZTEST(cicala_app_fsm, test_next_asks_for_a_question)
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

ZTEST(cicala_app_fsm, test_next_keeps_asking_from_the_same_deck)
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

ZTEST(cicala_app_fsm, test_changing_the_active_deck_announces_it)
{
    FakeIo io;
    TestAppFsm fsm(io);

    boot_to_question(fsm, io, 1);

    fsm.post_selector(3, true);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(REFRESHING));
    zassert_equal(io.labels, 2);
    zassert_equal(io.last_labelled_deck, 3);
    zassert_equal(io.draws, 1, "changing category does not request a question");
}

ZTEST(cicala_app_fsm, test_the_deck_name_stays_until_next)
{
    FakeIo io;
    TestAppFsm fsm(io);

    boot_to_question(fsm, io, 1);

    fsm.post_selector(5, true);
    settle(fsm);
    fsm.post_render(true);
    settle(fsm);

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

ZTEST(cicala_app_fsm, test_an_invalid_deck_keeps_what_is_on_the_panel)
{
    FakeIo io;
    TestAppFsm fsm(io);

    boot_to_question(fsm, io, 1);

    fsm.post_selector(0, false);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(SHOWING));
    zassert_equal(io.draws, 1, "an invalid value is not a deck change");
    zassert_equal(io.labels, 1);
}

ZTEST(cicala_app_fsm, test_a_press_during_a_refresh_is_dropped)
{
    FakeIo io;
    TestAppFsm fsm(io);

    boot_to_question(fsm, io, 1);

    fsm.post_next();
    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(REFRESHING));
    zassert_equal(io.draws, 2);

    fsm.post_next();
    settle(fsm);

    fsm.post_render(true);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(SHOWING));
    zassert_equal(io.draws, 2, "the press made during the refresh must not queue");
}

ZTEST(cicala_app_fsm, test_a_panel_that_never_answers_times_out)
{
    FakeIo io;
    TestAppFsm fsm(io);

    fsm.post_selector(0, true);
    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(REFRESHING));

    fsm.advance(CONFIG_CICALA_REFRESH_TIMEOUT_MS - 1);
    fsm.run();
    zassert_equal(fsm.get_current_state(), STATE(REFRESHING));

    fsm.advance(1);
    fsm.run();
    zassert_equal(fsm.get_current_state(), STATE(FAIL), "a dead panel must not wedge the device");

    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(SHOWING));
}

ZTEST(cicala_app_fsm, test_a_render_arriving_after_the_timeout_is_not_reused)
{
    FakeIo io;
    TestAppFsm fsm(io);

    fsm.post_selector(0, true);
    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(REFRESHING));

    fsm.advance(CONFIG_CICALA_REFRESH_TIMEOUT_MS);
    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(SHOWING));

    fsm.post_render(true);
    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(SHOWING), "a late render must not move anything");

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

ZTEST(cicala_app_fsm, test_a_failed_render_fails_without_waiting)
{
    FakeIo io;
    TestAppFsm fsm(io);

    fsm.post_selector(0, true);
    settle(fsm);

    fsm.post_render(false);
    fsm.run();

    zassert_equal(fsm.get_current_state(), STATE(FAIL));
}

ZTEST(cicala_app_fsm, test_an_empty_deck_fails_once_and_settles)
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

ZTEST(cicala_app_fsm, test_an_empty_deck_is_retried_on_the_next_press)
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

ZTEST(cicala_app_fsm, test_a_deck_name_that_cannot_be_shown_does_not_loop)
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

ZTEST(cicala_app_fsm, test_only_refreshing_asks_the_loop_to_stay_awake)
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

ZTEST(cicala_app_fsm, test_the_portal_gets_a_card_and_the_table_gets_it_back)
{
    FakeIo io;
    TestAppFsm fsm(io);

    fsm.post_selector(0, true);
    settle(fsm);
    fsm.post_render(true);
    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(SHOWING));

    fsm.post_service();
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(REFRESHING));
    zassert_equal(io.services, 1);
    zassert_equal(io.draws, 0, "a service card is not a question");

    fsm.post_render(true);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(SHOWING));
}

ZTEST(cicala_app_fsm, test_a_press_during_setup_still_asks_a_question)
{
    FakeIo io;
    TestAppFsm fsm(io);

    fsm.post_selector(0, true);
    settle(fsm);
    fsm.post_render(true);
    settle(fsm);

    fsm.post_service();
    settle(fsm);
    fsm.post_render(true);
    settle(fsm);

    fsm.post_next();
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(REFRESHING));
    zassert_equal(io.draws, 1);
}

ZTEST(cicala_app_fsm, test_a_card_the_panel_refuses_does_not_wedge_the_table)
{
    FakeIo io;
    TestAppFsm fsm(io);

    io.service_succeeds = false;

    fsm.post_selector(0, true);
    settle(fsm);
    fsm.post_render(true);
    settle(fsm);

    fsm.post_service();
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(SHOWING));
    zassert_equal(io.services, 1);

    fsm.post_next();
    settle(fsm);
    zassert_equal(io.draws, 1);
}

ZTEST(cicala_app_fsm, test_a_card_arriving_mid_refresh_waits_rather_than_being_dropped)
{
    FakeIo io;
    TestAppFsm fsm(io);

    fsm.post_selector(0, true);
    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(REFRESHING));

    fsm.post_service();
    settle(fsm);
    zassert_equal(io.services, 0, "not while the panel is busy");

    fsm.post_render(true);
    settle(fsm);

    zassert_equal(io.services, 1, "but as soon as it is free");
    zassert_equal(fsm.get_current_state(), STATE(REFRESHING));
}
