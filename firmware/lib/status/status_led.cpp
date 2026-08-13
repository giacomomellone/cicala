#include "status_led.hpp"

namespace tk
{

void StatusLed::arm(Colour colour, uint8_t blinks)
{
    _armed = true;
    _armed_colour = colour;
    _armed_blinks = blinks;
}

/*
 * Both refusing states blink, not just the critical one.
 *
 * LOW and CRITICAL both turn a press away — CONFIG_TK_REFRESH_MIN_MV is where
 * LOW begins — so a press in either has to be answered. Silence would be
 * indistinguishable from a dead button, and the panel cannot say anything
 * because saying it is a refresh. The two differ in how alarming they look.
 */
void StatusLed::set_power(PowerState state, bool refresh_blocked)
{
    const bool entered = state != _power;

    _power = state;

    if (!entered && !refresh_blocked) {
        return;
    }

    if (state == PowerState::CRITICAL) {
        arm(Colour::RED, 3);
    } else if (state == PowerState::LOW) {
        arm(Colour::AMBER, 1);
    }
}

void StatusLed::set_portal(bool on_air)
{
    _portal = on_air;
}

void StatusLed::set_activity(bool busy)
{
    _busy = busy;
}

Pattern StatusLed::steady() const
{
    if (_busy) {
        return {Colour::GREEN, Rhythm::PULSE, 0};
    }

    if (_portal) {
        return {Colour::AMBER, Rhythm::STEADY, 0};
    }

    switch (_power) {
    case PowerState::CHARGED:
        return {Colour::GREEN, Rhythm::STEADY, 0};

    case PowerState::CHARGING:
        return {Colour::RED, Rhythm::STEADY, 0};

    default:
        /*
         * Including LOW and CRITICAL. Their blink has already run by the time
         * anything reaches here, and a cell that is merely flat is not worth
         * lighting an LED about for the two seconds before the device sleeps —
         * it would spend the charge it is complaining about having none of.
         */
        return {Colour::OFF, Rhythm::STEADY, 0};
    }
}

/*
 * Mutating, despite reading like a query: this is where an armed burst is
 * stamped with the time it started. The setters run from zbus listeners and
 * have no clock to stamp it with.
 */
Pattern StatusLed::pattern(int64_t now_ms)
{
    if (_armed) {
        _armed = false;
        _burst_colour = _armed_colour;
        _burst_blinks = _armed_blinks;
        _burst_start_ms = now_ms;
        _burst_end_ms = now_ms + (int64_t) _armed_blinks * 2 * kStatusBlinkMs;
    }

    if (now_ms < _burst_end_ms) {
        const int64_t elapsed = now_ms - _burst_start_ms;
        const int64_t done = elapsed / (2 * kStatusBlinkMs);

        return {_burst_colour, Rhythm::BLINK, (uint8_t) (_burst_blinks - done)};
    }

    // Falling back rather than going dark. A burst that ended in darkness would
    // tell somebody mid-charge that their device had stopped charging.
    return steady();
}

Colour StatusLed::output(int64_t now_ms)
{
    const Pattern shown = pattern(now_ms);

    switch (shown.rhythm) {
    case Rhythm::BLINK: {
        const int64_t phase = (now_ms - _burst_start_ms) / kStatusBlinkMs;

        return (phase % 2) == 0 ? shown.colour : Colour::OFF;
    }

    case Rhythm::PULSE: {
        /*
         * Off the absolute clock rather than from when the transfer started.
         * Nothing can tell where a one-and-a-half second cycle began, and not
         * tracking it means one less thing to reset.
         */
        const int64_t phase = now_ms % kStatusPulseMs;

        return phase < (kStatusPulseMs / 2) ? shown.colour : Colour::OFF;
    }

    default:
        return shown.colour;
    }
}

bool StatusLed::animating(int64_t now_ms)
{
    return pattern(now_ms).rhythm != Rhythm::STEADY;
}

} // namespace tk
