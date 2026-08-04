/*
 * The e-paper panel. Renders one question and nothing else — no menus, no
 * status, no logo.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Bind the display and clear it.
 *
 * @return 0, or a negative errno if the display is missing or refuses to
 *         initialise.
 */
int tk_panel_init(void);

/**
 * Draw `text` and wait for the panel to finish.
 *
 * Blocks for the length of a refresh — 0.3 s partial, up to 2 s full. That is
 * why the display lives on its own thread: `app` has to stay responsive
 * enough to notice and drop a Next press made during it.
 *
 * @param text UTF-8, not NUL-terminated
 * @return 0 on success, -EINVAL when the text cannot be laid out, or the
 *         display driver's error.
 */
int tk_panel_render(const char *text, uint16_t len);

/**
 * Partial refreshes since the last full one.
 *
 * Exposed for the suite and for a future power bench; nothing in the
 * application reads it.
 */
uint16_t tk_panel_partial_count(void);

/** True when the next render will be a full refresh. */
bool tk_panel_next_is_full(void);

/**
 * Cell height of the font the last render chose, or 0 before the first one.
 *
 * Short questions are set in larger type so the panel is filled rather than
 * left with a band of white beneath them; this reports which size won.
 */
uint8_t tk_panel_last_font_height(void);

#ifdef __cplusplus
}
#endif
