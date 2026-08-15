/** The e-paper panel. */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Bind the display and reset the refresh policy. */
int kveld_panel_init(void);

/** Draw `text`; return 0 or a negative errno after the panel finishes. */
int kveld_panel_render(const char *text, uint16_t len);

/** Partial refreshes since the last full one. */
uint16_t kveld_panel_partial_count(void);

/** True when the next render will be a full refresh. */
bool kveld_panel_next_is_full(void);

/** Cell height of the font the last render chose, or 0 before the first one. */
uint8_t kveld_panel_last_font_height(void);

#ifdef __cplusplus
}
#endif
