/*
 * Table-driven finite state machine.
 *
 * A derived class supplies three things: an enum of states, an enum of
 * transitions, and a table mapping (current state, transition) to a next state
 * with an optional timeout. handle_current_state() then only has to answer
 * "what happened?" — never "where do I go?". The table is the specification.
 *
 *   1. Define the enums. CONTINUE / FAIL / REPEAT is a useful default set, but
 *      the values are yours; add your own where a state has more than one way
 *      to move on.
 *
 *   2. Write the table. STATE() and TRANSITION() cast the enums to int. Wrap it
 *      in `// clang-format off` so the column alignment survives.
 *
 *      // clang-format off
 *      const Fsm::StateTransition MyFsm::_transitions[] = {
 *      //   Current State     Transition             Next State        Timeout
 *          {STATE(IDLE),     TRANSITION(CONTINUE),  STATE(WORKING),   0       },
 *          {STATE(IDLE),     TRANSITION(REPEAT),    STATE(IDLE),      0       },
 *          {STATE(WORKING),  TRANSITION(CONTINUE),  STATE(IDLE),      0       },
 *          {STATE(WORKING),  TRANSITION(REPEAT),    STATE(WORKING),   30_s    },
 *      };
 *      // clang-format on
 *
 *   3. Implement get_fail_state() and handle_current_state(). Optionally
 *      override on_enter_state() / on_exit_state() for side effects, and
 *      now_ms() to control time (tests do this; see firmware/tests/fsm).
 *
 *   4. Call run() periodically. One call is one tick.
 *
 * Timeout semantics: a timeout only fires on a self-loop. A transition that
 * actually leaves the state always wins, even on the tick its timeout expires —
 * so a state cannot be sent to the fail state for succeeding a moment late.
 * A transition with no row in the table is a programming error and goes to the
 * fail state.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

/** Cast a State/Transition enum class value to the int the table stores. */
#define STATE(x) (static_cast<int>(State::x))
#define TRANSITION(x) (static_cast<int>(Transition::x))

namespace tk
{

constexpr int64_t operator""_ms(unsigned long long v)
{
    return static_cast<int64_t>(v);
}

constexpr int64_t operator""_s(unsigned long long v)
{
    return static_cast<int64_t>(v) * 1000;
}

class Fsm
{
public:
    virtual ~Fsm() = default;

    Fsm(const Fsm &) = delete;
    Fsm &operator=(const Fsm &) = delete;

    /** One tick: ask the derived class what happened, then apply the table. */
    void run();

    int get_current_state() const { return _current_state; }

    /** Milliseconds since the current state was entered. */
    int64_t get_elapsed_state_time() const;

    /**
     * True when any row for the current state carries a timeout.
     *
     * An event loop uses this to decide how long it may block: with no timeout
     * to honour it can wait forever, which is what lets the system reach deep
     * sleep. With one, it must wake up often enough to notice.
     */
    bool current_state_has_timeout() const;

    struct StateTransition {
        int current_state;
        int transition;
        int next_state;
        int64_t timeout_ms; ///< 0 means no timeout
    };

protected:
    Fsm(const StateTransition *transitions, size_t count, int initial_state);

    /** Where an undefined transition or an expired self-loop lands. */
    virtual int get_fail_state() const = 0;

    /** What happened since the last tick. Returns a Transition value. */
    virtual int handle_current_state() = 0;

    virtual void on_enter_state(int state) { (void) state; }

    virtual void on_exit_state(int state) { (void) state; }

    /**
     * Current time in milliseconds. The default is the Zephyr uptime; tests
     * override it so timeouts can be exercised without sleeping.
     */
    virtual int64_t now_ms() const;

private:
    const StateTransition *find_transition(int transition) const;
    bool has_current_state_exceeded_timeout() const;
    void perform_transition(int next_state);

    const StateTransition *_transitions;
    size_t _transitions_count;
    int _current_state;
    int64_t _state_entry_ms;
    bool _entered; ///< false until the first run(); see the constructor
};

} // namespace tk
