/*
 * Fsm behaviour. Time is injected, so a 30-second timeout is tested in
 * microseconds and the suite never sleeps.
 */

#include <zephyr/ztest.h>

#include "fsm.hpp"

using namespace tk;

namespace
{

enum class State { IDLE = 0, WORKING, DONE, FAILED };
enum class Transition { CONTINUE, FAIL, REPEAT };

/** A machine whose tick result and clock the test drives directly. */
class TestFsm : public Fsm
{
public:
    // Defined below the table: ARRAY_SIZE needs a complete array type.
    TestFsm();

    int next_transition = TRANSITION(REPEAT);
    int64_t clock = 0;

    int enter_count = 0;
    int exit_count = 0;
    int last_entered = -1;
    int last_exited = -1;

    void advance(int64_t ms) { clock += ms; }

protected:
    int get_fail_state() const override { return STATE(FAILED); }

    int handle_current_state() override { return next_transition; }

    int64_t now_ms() const override { return clock; }

    void on_enter_state(int state) override
    {
        enter_count++;
        last_entered = state;
    }

    void on_exit_state(int state) override
    {
        exit_count++;
        last_exited = state;
    }

private:
    static const Fsm::StateTransition _transitions[];
};

// clang-format off
const Fsm::StateTransition TestFsm::_transitions[] = {
//   Current State      Transition             Next State        Timeout
    {STATE(IDLE),      TRANSITION(CONTINUE),  STATE(WORKING),   0       },
    {STATE(IDLE),      TRANSITION(REPEAT),    STATE(IDLE),      0       },

    {STATE(WORKING),   TRANSITION(CONTINUE),  STATE(DONE),      0       },
    {STATE(WORKING),   TRANSITION(REPEAT),    STATE(WORKING),   30_s    },

    {STATE(DONE),      TRANSITION(REPEAT),    STATE(DONE),      0       },

    {STATE(FAILED),    TRANSITION(CONTINUE),  STATE(IDLE),      0       },
    {STATE(FAILED),    TRANSITION(REPEAT),    STATE(FAILED),    0       },
};
// clang-format on

TestFsm::TestFsm() : Fsm(_transitions, ARRAY_SIZE(_transitions), STATE(IDLE)) {}

} // namespace

ZTEST_SUITE(tk_fsm, NULL, NULL, NULL, NULL, NULL);

ZTEST(tk_fsm, test_starts_in_initial_state)
{
    TestFsm fsm;

    zassert_equal(fsm.get_current_state(), STATE(IDLE));
}

ZTEST(tk_fsm, test_enters_initial_state_on_first_tick)
{
    TestFsm fsm;

    // on_enter_state is virtual, so it cannot run from the constructor.
    zassert_equal(fsm.enter_count, 0, "entered before the first tick");

    fsm.run();
    zassert_equal(fsm.enter_count, 1);
    zassert_equal(fsm.last_entered, STATE(IDLE));
}

ZTEST(tk_fsm, test_continue_moves_to_next_state)
{
    TestFsm fsm;

    fsm.next_transition = TRANSITION(CONTINUE);
    fsm.run();

    zassert_equal(fsm.get_current_state(), STATE(WORKING));
    zassert_equal(fsm.last_exited, STATE(IDLE));
    zassert_equal(fsm.last_entered, STATE(WORKING));
}

ZTEST(tk_fsm, test_repeat_stays_without_re_entering)
{
    TestFsm fsm;

    fsm.run();
    const int entered_once = fsm.enter_count;

    fsm.run();
    fsm.run();

    zassert_equal(fsm.get_current_state(), STATE(IDLE));
    zassert_equal(fsm.enter_count, entered_once, "self-loop re-entered the state");
    zassert_equal(fsm.exit_count, 0, "self-loop exited the state");
}

ZTEST(tk_fsm, test_repeat_does_not_restart_the_timeout_clock)
{
    TestFsm fsm;

    fsm.next_transition = TRANSITION(CONTINUE);
    fsm.run(); // -> WORKING, 30 s timeout

    fsm.next_transition = TRANSITION(REPEAT);
    for (int i = 0; i < 10; i++) {
        fsm.advance(1_s);
        fsm.run();
    }

    // If a self-loop reset the entry time, elapsed would be 1 s, not 10.
    zassert_equal(fsm.get_elapsed_state_time(), 10_s);
    zassert_equal(fsm.get_current_state(), STATE(WORKING));
}

ZTEST(tk_fsm, test_self_loop_times_out_to_fail_state)
{
    TestFsm fsm;

    fsm.next_transition = TRANSITION(CONTINUE);
    fsm.run(); // -> WORKING

    fsm.next_transition = TRANSITION(REPEAT);
    fsm.advance(29_s);
    fsm.run();
    zassert_equal(fsm.get_current_state(), STATE(WORKING), "timed out early");

    fsm.advance(1_s + 1_ms);
    fsm.run();
    zassert_equal(fsm.get_current_state(), STATE(FAILED));
}

ZTEST(tk_fsm, test_leaving_transition_beats_an_expired_timeout)
{
    // Regression: the original checked the timeout before looking for a
    // transition, so a state that legitimately succeeded on the same tick its
    // timeout expired was sent to the fail state instead of its real target.
    TestFsm fsm;

    fsm.next_transition = TRANSITION(CONTINUE);
    fsm.run(); // -> WORKING

    fsm.advance(60_s); // well past the 30 s timeout
    fsm.next_transition = TRANSITION(CONTINUE);
    fsm.run();

    zassert_equal(fsm.get_current_state(), STATE(DONE), "expired timeout stole a valid transition");
}

ZTEST(tk_fsm, test_undefined_transition_goes_to_fail_state)
{
    TestFsm fsm;

    // DONE has only a REPEAT row; CONTINUE from there is a table bug.
    fsm.next_transition = TRANSITION(CONTINUE);
    fsm.run(); // IDLE -> WORKING
    fsm.run(); // WORKING -> DONE
    zassert_equal(fsm.get_current_state(), STATE(DONE));

    fsm.run();
    zassert_equal(fsm.get_current_state(), STATE(FAILED));
}

ZTEST(tk_fsm, test_fail_transition_uses_the_table_not_the_fail_state)
{
    // FAIL is an ordinary transition value. It only reaches the fail state if
    // the table says so, or if no row matches at all.
    TestFsm fsm;

    fsm.next_transition = TRANSITION(FAIL);
    fsm.run(); // IDLE has no FAIL row -> fail state

    zassert_equal(fsm.get_current_state(), STATE(FAILED));
}

ZTEST(tk_fsm, test_current_state_has_timeout)
{
    TestFsm fsm;

    // IDLE's rows all carry timeout 0, so an event loop may block forever.
    zassert_false(fsm.current_state_has_timeout());

    fsm.next_transition = TRANSITION(CONTINUE);
    fsm.run(); // -> WORKING, which has a 30 s row

    zassert_true(fsm.current_state_has_timeout());
}

ZTEST(tk_fsm, test_elapsed_time_resets_on_a_real_transition)
{
    TestFsm fsm;

    fsm.run();
    fsm.advance(5_s);
    zassert_equal(fsm.get_elapsed_state_time(), 5_s);

    fsm.next_transition = TRANSITION(CONTINUE);
    fsm.run(); // -> WORKING

    zassert_equal(fsm.get_elapsed_state_time(), 0);
}
