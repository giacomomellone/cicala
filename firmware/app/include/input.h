/*
 * The Category and Next buttons. Turns debounced key events into the two input
 * channels in channels.h and owns no policy of its own.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Check both buttons are present and start reporting.
 *
 * @return 0, or a negative errno if a button pin is not ready.
 */
int tk_input_init(void);

#ifdef __cplusplus
}
#endif
