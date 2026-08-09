/*
 * C face of the LED arbiter.
 *
 * `status_logic.cpp` holds the StatusLed from lib/status; `status.c` owns the
 * two pins, the work item and the zbus listener, because the observer macros do
 * not compile as C++. The seam, the same way net_logic.h and power_logic.h are.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * What the two pins should be doing. Same order as `tk::Colour`, and
 * status_logic.cpp asserts it.
 */
enum tk_status_colour {
    TK_STATUS_OFF = 0,
    TK_STATUS_RED,
    TK_STATUS_GREEN,
    /** Both lit. One package on the target board, two on the bench. */
    TK_STATUS_AMBER,
};

/** Where power stands, plus whether a press was just turned away. */
void tk_status_post_power(uint8_t state, bool refresh_blocked);

void tk_status_post_portal(bool on_air);
void tk_status_post_activity(bool busy);

/** The instantaneous colour, with blinks and pulses already resolved. */
uint8_t tk_status_output(int64_t now_ms);

/** True while the output can change without any new input arriving. */
bool tk_status_animating(int64_t now_ms);

#ifdef __cplusplus
}
#endif
