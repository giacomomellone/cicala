/*
 * Selector and Next button. Turns GPIO edges into the two input channels in
 * channels.h; owns no policy beyond "what counts as a deck change".
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Seed the selector from the pins and start reporting.
 *
 * The gpio-keys driver only reports *edges*, and it samples each pin once at
 * init to seed its own state, so a contact that is already closed at boot
 * never produces an event. Reading the pins here is what makes "read the
 * selector at boot" true — the wake path in docs/firmware_architecture.md
 * depends on it, since after deep sleep every boot is a cold one.
 *
 * @return 0, or a negative errno if a selector pin is unreadable.
 */
int tk_input_init(void);

#ifdef __cplusplus
}
#endif
