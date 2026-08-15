/** C face of the LED arbiter. */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** What the two pins should be doing. */
enum kveld_status_colour {
    KVELD_STATUS_OFF = 0,
    KVELD_STATUS_RED,
    KVELD_STATUS_GREEN,
    /** Both lit. */
    KVELD_STATUS_AMBER,
};

/** Where power stands, plus whether a press was just turned away. */
void kveld_status_post_power(uint8_t state, bool refresh_blocked);

void kveld_status_post_portal(bool on_air);
void kveld_status_post_activity(bool busy);

/** The instantaneous colour, with blinks and pulses already resolved. */
uint8_t kveld_status_output(int64_t now_ms);

/** True while the output can change without any new input arriving. */
bool kveld_status_animating(int64_t now_ms);

#ifdef __cplusplus
}
#endif
