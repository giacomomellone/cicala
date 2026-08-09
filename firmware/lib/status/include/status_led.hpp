/*
 * What the two LEDs should be doing, and why that rather than something else.
 *
 * No Zephyr headers, no pins, no timers: inputs are pushed in with set_*(), the
 * clock arrives as an argument, and the answer comes back as a Pattern. That is
 * what lets the suite check a priority order and a blink count without a board.
 *
 * ## Two colours, so rhythm carries as much as hue
 *
 * The device has a red LED and a green one — one package on the target board,
 * two on the bench. Lighting both is amber. That is the whole palette, and it
 * has to cover more conditions than it has colours, so two of them are told
 * apart by how they move rather than by what colour they are: a low-battery
 * blink against a steady portal amber, and a transfer against a charged cell.
 *
 * ## Two layers
 *
 * A steady colour says what is true for minutes or hours — charging, charged,
 * a portal on air. A transient preempts it for a moment and then falls back:
 * three red blinks when a press was refused, one amber when the cell first
 * drops under the floor. Falling back matters. A blink that ended by going dark
 * would tell somebody charging their device that it had stopped.
 *
 * ## What is deliberately not here
 *
 * A failed ADC reading. It is a bench condition, the console already reports
 * it, and a fourth thing to distinguish would make the other three harder to
 * read across a table.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "power_state.hpp"

namespace tk
{

#ifdef CONFIG_TK_STATUS_LED_BLINK_MS
constexpr int64_t kStatusBlinkMs = CONFIG_TK_STATUS_LED_BLINK_MS;
#else
constexpr int64_t kStatusBlinkMs = 200;
#endif

#ifdef CONFIG_TK_STATUS_LED_PULSE_MS
constexpr int64_t kStatusPulseMs = CONFIG_TK_STATUS_LED_PULSE_MS;
#else
constexpr int64_t kStatusPulseMs = 1500;
#endif

/** Every colour two LEDs can make. AMBER is both of them lit. */
enum class Colour {
    OFF = 0,
    RED,
    GREEN,
    AMBER,
};

enum class Rhythm {
    /** On, and staying on. */
    STEADY = 0,
    /** A counted number of on-off pairs, then back to whatever was underneath. */
    BLINK,
    /**
     * A slow on-off, for as long as the condition lasts.
     *
     * Not a fade. Two GPIOs with series resistors have two brightnesses, so
     * this is a long blink; a board with PWM on these pins could breathe
     * instead without anything here changing.
     */
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
    /**
     * Where power stands, and whether a refresh was just refused.
     *
     * `refresh_blocked` is an event rather than a condition: pass true on the
     * press that was turned away, not for as long as the cell is flat. It is
     * what fires the blink, and it fires it again on the next press, because
     * somebody pressing a second time deserves the same answer as the first.
     */
    void set_power(PowerState state, bool refresh_blocked);

    /** The setup portal is on air. */
    void set_portal(bool on_air);

    /**
     * A sync or an update is moving bytes.
     *
     * One flag for both: they look the same because the panel names which one
     * is running, and inventing a second rhythm to separate them would cost
     * legibility for information nobody is missing.
     */
    void set_activity(bool busy);

    /** What should be showing: colour, rhythm, and any blinks left to run. */
    Pattern pattern(int64_t now_ms);

    /** The same thing resolved to this instant — what the pins should be. */
    Colour output(int64_t now_ms);

    /**
     * True when output() can change on its own.
     *
     * A caller with a work queue uses this to decide whether to come back:
     * a steady colour needs nothing until an input changes.
     */
    bool animating(int64_t now_ms);

private:
    /** Arm a burst. Stamped with a time on the next pattern() call. */
    void arm(Colour colour, uint8_t blinks);

    /** Resolve the steady layer: activity, then portal, then power. */
    Pattern steady() const;

    PowerState _power = PowerState::UNKNOWN;
    bool _portal = false;
    bool _busy = false;

    /*
     * Armed here, started in pattern(). The setters run from zbus listeners
     * and have no business reading a clock, and a burst that started at the
     * moment it was armed would have to be given one.
     */
    bool _armed = false;
    Colour _armed_colour = Colour::OFF;
    uint8_t _armed_blinks = 0;

    Colour _burst_colour = Colour::OFF;
    uint8_t _burst_blinks = 0;
    int64_t _burst_start_ms = 0;
    int64_t _burst_end_ms = 0;
};

} // namespace tk
