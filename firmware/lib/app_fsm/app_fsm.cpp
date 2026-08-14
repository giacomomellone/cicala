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
    // Consume the Next press that woke the device.
    {STATE(BOOT),        TRANSITION(REDRAW),     STATE(DRAWING),      0       },
    // A cold boot shows the active deck first.
    {STATE(BOOT),        TRANSITION(CONTINUE),   STATE(CATEGORY),     0       },

    {STATE(CATEGORY),    TRANSITION(CONTINUE),   STATE(REFRESHING),   0       },
    {STATE(CATEGORY),    TRANSITION(FAILED),     STATE(FAIL),         0       },

    {STATE(SHOWING),     TRANSITION(REPEAT),     STATE(SHOWING),      0       },
    {STATE(SHOWING),     TRANSITION(REDRAW),     STATE(DRAWING),      0       },
    {STATE(SHOWING),     TRANSITION(RELABEL),    STATE(CATEGORY),     0       },
    {STATE(SHOWING),     TRANSITION(SERVICE),    STATE(SERVICE),      0       },

    {STATE(DRAWING),     TRANSITION(CONTINUE),   STATE(REFRESHING),   0       },
    {STATE(DRAWING),     TRANSITION(FAILED),     STATE(FAIL),         0       },

    // Bound a stalled display refresh.
    {STATE(REFRESHING),  TRANSITION(REPEAT),     STATE(REFRESHING),   kRefreshTimeoutMs},
    {STATE(REFRESHING),  TRANSITION(CONTINUE),   STATE(SHOWING),      0       },
    {STATE(REFRESHING),  TRANSITION(FAILED),     STATE(FAIL),         0       },

    {STATE(SERVICE),     TRANSITION(CONTINUE),   STATE(REFRESHING),   0       },
    {STATE(SERVICE),     TRANSITION(FAILED),     STATE(FAIL),         0       },

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
    // A new card invalidates results from the previous refresh.
    switch (state) {
    case STATE(DRAWING):
        _render_pending = false;
        _draw_ok = _io.draw(_selector_deck);

        if (_draw_ok) {
            _shown_deck = _selector_deck;
        }

        break;

    case STATE(CATEGORY):
        _render_pending = false;
        _label_ok = _io.show_category(_selector_deck);

        // Prevent a retry loop after a failed announcement.
        _shown_deck = _selector_deck;
        break;

    case STATE(SERVICE):
        _render_pending = false;
        _service_ok = _io.show_service();
        break;

    case STATE(FAIL):
        // Keep an empty deck from retrying until another input arrives.
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

    case STATE(CATEGORY):
        return on_category();

    case STATE(SHOWING):
        return on_showing();

    case STATE(DRAWING):
        return on_drawing();

    case STATE(REFRESHING):
        return on_refreshing();

    case STATE(SERVICE):
        return on_service();

    case STATE(FAIL):
    default:
        return on_fail();
    }
}

int AppFsm::on_boot()
{
    // Keep the panel unchanged until one deck is selected.
    if (!_selector_valid) {
        return TRANSITION(REPEAT);
    }

    // A wake press takes precedence over the retained panel.
    if (_next_pending) {
        _next_pending = false;
        _shown_deck = _selector_deck;

        return TRANSITION(REDRAW);
    }

    if (_io.retained_matches(_selector_deck)) {
        _shown_deck = _selector_deck;

        return TRANSITION(RETAINED);
    }

    return TRANSITION(CONTINUE);
}

int AppFsm::on_category()
{
    return _label_ok ? TRANSITION(CONTINUE) : TRANSITION(FAILED);
}

int AppFsm::on_showing()
{
    if (!_selector_valid) {
        return TRANSITION(REPEAT);
    }

    // Service status takes priority over pending tabletop input.
    if (_service_pending) {
        _service_pending = false;

        return TRANSITION(SERVICE);
    }

    // Next always requests a question from the current deck.
    if (_next_pending) {
        _next_pending = false;

        return TRANSITION(REDRAW);
    }

    if (_selector_deck != _shown_deck) {
        return TRANSITION(RELABEL);
    }

    return TRANSITION(REPEAT);
}

int AppFsm::on_drawing()
{
    return _draw_ok ? TRANSITION(CONTINUE) : TRANSITION(FAILED);
}

int AppFsm::on_refreshing()
{
    // Presses during a refresh are dropped.
    _next_pending = false;

    if (_render_pending) {
        _render_pending = false;

        return _render_ok ? TRANSITION(CONTINUE) : TRANSITION(FAILED);
    }

    return TRANSITION(REPEAT);
}

int AppFsm::on_service()
{
    return _service_ok ? TRANSITION(CONTINUE) : TRANSITION(FAILED);
}

int AppFsm::on_fail()
{
    return TRANSITION(CONTINUE);
}

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

void AppFsm::post_service()
{
    _service_pending = true;
}

} // namespace tk
