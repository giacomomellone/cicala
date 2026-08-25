/* Priority and timing rules for the red and green status LEDs. */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "power_state.hpp"

namespace cicala
{

#ifdef CONFIG_CICALA_STATUS_LED_BLINK_MS
constexpr int64_t kStatusBlinkMs = CONFIG_CICALA_STATUS_LED_BLINK_MS;
#else
constexpr int64_t kStatusBlinkMs = 200;
#endif

#ifdef CONFIG_CICALA_STATUS_LED_PULSE_MS
constexpr int64_t kStatusPulseMs = CONFIG_CICALA_STATUS_LED_PULSE_MS;
#else
constexpr int64_t kStatusPulseMs = 1500;
#endif

/** AMBER lights both LEDs. */
enum class Colour {
    OFF = 0,
    RED,
    GREEN,
    AMBER,
};

enum class Rhythm {
    STEADY = 0,
    /** A finite burst, followed by the steady state. */
    BLINK,
    /** A slow on-off cycle while the condition remains active. */
    PULSE,
};

struct Pattern {
    Colour colour;
    Rhythm rhythm;
    /** Blinks remaining in this burst. Zero for STEADY and PULSE. */
    uint8_t count;
};

class StatusLed
{
public:
    /** Update power state. `refresh_blocked` fires one refusal burst. */
    void set_power(PowerState state, bool refresh_blocked);

    /** The setup portal is on air. */
    void set_portal(bool on_air);

    /** Set whether sync or update traffic is active. */
    void set_activity(bool busy);

    /** Return the current colour, rhythm, and remaining blink count. */
    Pattern pattern(int64_t now_ms);

    /** Resolve the pattern to an LED colour at `now_ms`. */
    Colour output(int64_t now_ms);

    /** Return whether output() can change without new input. */
    bool animating(int64_t now_ms);

private:
    /** Arm a burst. Stamped with a time on the next pattern() call. */
    void arm(Colour colour, uint8_t blinks);

    /** Resolve the steady layer in priority order. */
    Pattern steady() const;

    PowerState _power = PowerState::UNKNOWN;
    bool _portal = false;
    bool _busy = false;

    // pattern() records the start time after a setter arms the burst.
    bool _armed = false;
    Colour _armed_colour = Colour::OFF;
    uint8_t _armed_blinks = 0;

    Colour _burst_colour = Colour::OFF;
    uint8_t _burst_blinks = 0;
    int64_t _burst_start_ms = 0;
    int64_t _burst_end_ms = 0;
};

} // namespace cicala
