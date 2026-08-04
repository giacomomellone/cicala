#include "app_fsm.hpp"

#include <zephyr/sys/util.h>

namespace tk
{

#ifdef CONFIG_TK_REFRESH_TIMEOUT_MS
constexpr int64_t kRefreshTimeoutMs = CONFIG_TK_REFRESH_TIMEOUT_MS;
#else
constexpr int64_t kRefreshTimeoutMs = 15000;
#endif

// clang-format off
const Fsm::StateTransition AppFsm::_transitions[] = {
//   Current State        Transition              Next State           Timeout
    {STATE(BOOT),        TRANSITION(REPEAT),     STATE(BOOT),         0       },
    {STATE(BOOT),        TRANSITION(RETAINED),   STATE(SHOWING),      0       },
    {STATE(BOOT),        TRANSITION(CONTINUE),   STATE(DRAWING),      0       },

    {STATE(SHOWING),     TRANSITION(REPEAT),     STATE(SHOWING),      0       },
    {STATE(SHOWING),     TRANSITION(REDRAW),     STATE(DRAWING),      0       },

    {STATE(DRAWING),     TRANSITION(CONTINUE),   STATE(REFRESHING),   0       },
    {STATE(DRAWING),     TRANSITION(FAILED),     STATE(FAIL),         0       },

    // A dead panel must not wedge the device, so this one has a timeout. It
    // has to clear the slowest real refresh by a wide margin: a full update on
    // the GDEY0213B74 takes seconds, and a timeout that fires during an
    // ordinary refresh is far worse than one that fires late.
    {STATE(REFRESHING),  TRANSITION(REPEAT),     STATE(REFRESHING),   kRefreshTimeoutMs},
    {STATE(REFRESHING),  TRANSITION(CONTINUE),   STATE(SHOWING),      0       },
    {STATE(REFRESHING),  TRANSITION(FAILED),     STATE(FAIL),         0       },

    {STATE(FAIL),        TRANSITION(CONTINUE),   STATE(SHOWING),      0       },
    {STATE(FAIL),        TRANSITION(REPEAT),     STATE(FAIL),         0       },
};
// clang-format on

AppFsm::AppFsm(AppIo &io) : Fsm(_transitions, ARRAY_SIZE(_transitions), STATE(BOOT)), _io(io) {}

int AppFsm::get_fail_state() const
{
    return STATE(FAIL);
}

void AppFsm::on_enter_state(int state)
{
    switch (state) {
    case STATE(DRAWING):
        /*
         * Forget any render result still sitting here. It can only belong to
         * an earlier question — the one whose refresh timed out, arriving
         * after the machine had already given up on it. Left in place it
         * satisfies the *next* refresh the instant that one starts, so the
         * panel is told to draw and the machine calls it done in the same
         * breath. On the bench that looks like one press doing nothing and
         * the next showing two questions in quick succession.
         */
        _render_pending = false;
        _draw_ok = _io.draw(_selector_deck);

        if (_draw_ok) {
            _shown_deck = _selector_deck;
        }

        break;

    case STATE(FAIL):
        // Claim the deck even though nothing was drawn. Without this, SHOWING
        // sees a deck it has not shown, asks for a draw, fails again, and the
        // device spins between three states forever. A deck that yields nothing
        // should sit quietly on the previous question until the user does
        // something — a press, or a turn of the selector.
        _shown_deck = _selector_deck;
        break;

    default:
        break;
    }
}

int AppFsm::handle_current_state()
{
    switch (get_current_state()) {
    case STATE(BOOT):
        return on_boot();

    case STATE(SHOWING):
        return on_showing();

    case STATE(DRAWING):
        return on_drawing();

    case STATE(REFRESHING):
        return on_refreshing();

    case STATE(FAIL):
    default:
        return on_fail();
    }
}

int AppFsm::on_boot()
{
    // A broken or mid-travel selector waits here indefinitely, with the panel
    // left readable. There is nothing better to do than keep the last question.
    if (!_selector_valid) {
        return TRANSITION(REPEAT);
    }

    if (_io.retained_matches(_selector_deck)) {
        // Nothing to draw and nothing to refresh: the panel is already right.
        // Adopt what it shows so SHOWING does not think otherwise.
        _shown_deck = _selector_deck;

        return TRANSITION(RETAINED);
    }

    return TRANSITION(CONTINUE);
}

int AppFsm::on_showing()
{
    // Zero or several contacts is not a deck. Keep the question.
    if (!_selector_valid) {
        return TRANSITION(REPEAT);
    }

    if (_next_pending) {
        _next_pending = false;

        return TRANSITION(REDRAW);
    }

    if (_selector_deck != _shown_deck) {
        return TRANSITION(REDRAW);
    }

    return TRANSITION(REPEAT);
}

int AppFsm::on_drawing()
{
    // The draw itself happened in on_enter_state(); this only reports how it
    // went, which keeps the one action out of a function called every tick.
    return _draw_ok ? TRANSITION(CONTINUE) : TRANSITION(FAILED);
}

int AppFsm::on_refreshing()
{
    // The press that arrives mid-refresh is dropped rather than queued. A panel
    // takes up to two seconds; honouring presses made during it would spend that
    // time drawing questions nobody has read yet.
    _next_pending = false;

    if (_render_pending) {
        _render_pending = false;

        return _render_ok ? TRANSITION(CONTINUE) : TRANSITION(FAILED);
    }

    return TRANSITION(REPEAT);
}

int AppFsm::on_fail()
{
    return TRANSITION(CONTINUE);
}

////////////////////// Input //////////////////////

void AppFsm::post_selector(uint8_t deck, bool valid)
{
    _selector_deck = deck;
    _selector_valid = valid;
}

void AppFsm::post_next()
{
    _next_pending = true;
}

void AppFsm::post_render(bool ok)
{
    _render_pending = true;
    _render_ok = ok;
}

} // namespace tk
