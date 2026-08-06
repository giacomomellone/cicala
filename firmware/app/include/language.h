/*
 * Which language the device asks its questions in.
 *
 * The image carries every shipped corpus, so this is a choice the device can
 * actually act on rather than a preference recorded for later. It survives a
 * power cycle in the same NVS the Wi-Fi credentials use, and falls back to
 * CONFIG_TK_CORPUS_LANGUAGE when nothing has been chosen — which is what a
 * device that has never seen the setup portal is.
 *
 * Without CONFIG_SETTINGS there is nowhere to keep it, so the compiled-in
 * language is the only answer and setting it fails. That is the qemu and
 * native_sim case, where the suites run.
 */

#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Two letters and a NUL. */
#define TK_LANGUAGE_LEN 3

/**
 * The language in use, as a two-letter code.
 *
 * Never NULL, and valid for the life of the program.
 */
const char *tk_language(void);

/**
 * Remember `code` as the language to ask questions in.
 *
 * Only the codes the image actually carries are accepted; anything else is
 * refused rather than stored, because a stored language with no corpus behind
 * it would leave the device with nothing to draw.
 *
 * Does not reopen the corpus. The caller publishes on chan_corpus and `app`
 * does that, so the change lands on the next requested draw rather than
 * interrupting whatever is on the panel.
 *
 * @return 0, -EINVAL for a language the image does not carry, or a negative
 *         errno from the settings backend.
 */
int tk_language_set(const char *code);

/** True when `code` names a corpus compiled into this image. */
bool tk_language_available(const char *code);

#ifdef __cplusplus
}
#endif
