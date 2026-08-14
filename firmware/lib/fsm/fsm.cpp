#include "fsm.hpp"

#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>

namespace tk
{

Fsm::Fsm(const StateTransition *transitions, size_t count, int initial_state)
    : _transitions(transitions), _transitions_count(count), _current_state(initial_state),
      _state_entry_ms(0), _entered(false)
{
    __ASSERT_NO_MSG(transitions != nullptr);
    __ASSERT_NO_MSG(count > 0);
    // Defer virtual callbacks until the first tick. Uptime may be zero.
}

int64_t Fsm::now_ms() const
{
    return k_uptime_get();
}

void Fsm::run()
{
    if (!_entered) {
        _entered = true;
        _state_entry_ms = now_ms();
        on_enter_state(_current_state);
    }

    const int transition = handle_current_state();
    const StateTransition *tr = find_transition(transition);

    if (tr == nullptr) {
        perform_transition(get_fail_state());
        return;
    }

    // Only self-loops can time out.
    if (tr->next_state == _current_state && has_current_state_exceeded_timeout()) {
        perform_transition(get_fail_state());
        return;
    }

    perform_transition(tr->next_state);
}

int64_t Fsm::get_elapsed_state_time() const
{
    return now_ms() - _state_entry_ms;
}

bool Fsm::current_state_has_timeout() const
{
    for (size_t i = 0; i < _transitions_count; ++i) {
        const StateTransition &tr = _transitions[i];

        if (tr.current_state == _current_state && tr.timeout_ms > 0) {
            return true;
        }
    }

    return false;
}

const Fsm::StateTransition *Fsm::find_transition(int transition) const
{
    for (size_t i = 0; i < _transitions_count; ++i) {
        const StateTransition &tr = _transitions[i];

        if (tr.current_state == _current_state && tr.transition == transition) {
            return &tr;
        }
    }

    return nullptr;
}

bool Fsm::has_current_state_exceeded_timeout() const
{
    for (size_t i = 0; i < _transitions_count; ++i) {
        const StateTransition &tr = _transitions[i];

        if (tr.current_state == _current_state && tr.timeout_ms > 0 &&
            get_elapsed_state_time() >= tr.timeout_ms) {
            return true;
        }
    }

    return false;
}

void Fsm::perform_transition(int next_state)
{
    if (_current_state == next_state) {
        // Self-loops do not restart their timeout.
        return;
    }

    on_exit_state(_current_state);
    _current_state = next_state;
    _state_entry_ms = now_ms();
    on_enter_state(_current_state);
}

} // namespace tk
