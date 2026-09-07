#include "status_led.hpp"

namespace cicala
{

void StatusLed::arm(Colour colour, uint8_t blinks)
{
    _armed = true;
    _armed_colour = colour;
    _armed_blinks = blinks;
}

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
    if (_power == PowerState::CHARGE_FAULT) {
        return {Colour::RED, Rhythm::PULSE, 0};
    }

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

    case PowerState::EXTERNAL_IDLE:
        return {Colour::AMBER, Rhythm::PULSE, 0};

    default:
        return {Colour::OFF, Rhythm::STEADY, 0};
    }
}

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

    // Resume the steady power state after a burst.
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

} // namespace cicala
