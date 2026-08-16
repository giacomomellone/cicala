/* Table-driven state machine. Only self-loop transitions can time out. */

#pragma once

#include <stddef.h>
#include <stdint.h>

#define STATE(x) (static_cast<int>(State::x))
#define TRANSITION(x) (static_cast<int>(Transition::x))

namespace kveld
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

    /** Process one state-machine tick. */
    void run();

    int get_current_state() const { return _current_state; }

    /** Milliseconds since the current state was entered. */
    int64_t get_elapsed_state_time() const;

    /** Return whether the current state has a timeout. */
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

    /** Return the transition observed since the last tick. */
    virtual int handle_current_state() = 0;

    virtual void on_enter_state(int state) { (void) state; }

    virtual void on_exit_state(int state) { (void) state; }

    /** Current time in milliseconds. Tests may override the Zephyr uptime. */
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

} // namespace kveld
