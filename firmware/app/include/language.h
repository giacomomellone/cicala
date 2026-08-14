/** Which language the device asks its questions in. */

#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Two letters and a NUL. */
#define TK_LANGUAGE_LEN 3

/** The language in use, as a two-letter code. */
const char *tk_language(void);

/** Remember `code` as the language to ask questions in. */
int tk_language_set(const char *code);

/** True when `code` names a corpus compiled into this image. */
bool tk_language_available(const char *code);

#ifdef __cplusplus
}
#endif
