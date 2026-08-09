/*
 * What the cell is doing, as one enum with no dependencies.
 *
 * Separate from power_fsm.hpp so `lib/status` can name a power state without
 * pulling in the state machine that produces one. The C side of the same
 * vocabulary is `enum tk_power_state` in app/include/channels.h, and the two
 * are kept in the same order on purpose — power_logic.cpp casts between them.
 */

#pragma once

namespace tk
{

enum class PowerState {
    /**
     * Nothing has been measured yet, or the last thing measured made no sense.
     *
     * Permits refreshes. A broken ADC must not stop the device drawing
     * questions: the panel is the product and the battery reading is advice.
     */
    UNKNOWN = 0,
    /** On the cell, above the refresh floor. */
    NORMAL,
    /** On the cell, under the refresh floor. Refreshes are refused. */
    LOW,
    /** On the cell, nearly flat. Refreshes are refused and said so, loudly. */
    CRITICAL,
    /** External power is in. Says nothing about whether current is flowing. */
    CHARGING,
    /**
     * External power is in and the cell reads full.
     *
     * An estimate, not a termination signal — see power_fsm.hpp.
     */
    CHARGED,
};

} // namespace tk
