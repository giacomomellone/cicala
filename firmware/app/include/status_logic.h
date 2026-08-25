/** C face of the LED arbiter. */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** What the two pins should be doing. */
enum cicala_status_colour {
    CICALA_STATUS_OFF = 0,
    CICALA_STATUS_RED,
    CICALA_STATUS_GREEN,
    /** Both lit. */
    CICALA_STATUS_AMBER,
};

/** Where power stands, plus whether a press was just turned away. */
void cicala_status_post_power(uint8_t state, bool refresh_blocked);

void cicala_status_post_portal(bool on_air);
void cicala_status_post_activity(bool busy);

/** The instantaneous colour, with blinks and pulses already resolved. */
uint8_t cicala_status_output(int64_t now_ms);

/** True while the output can change without any new input arriving. */
bool cicala_status_animating(int64_t now_ms);

#ifdef __cplusplus
}
#endif
