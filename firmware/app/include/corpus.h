/*
 * The question corpora compiled into this image.
 *
 * One per shipped language, embedded by app/CMakeLists.txt from the bundles
 * `just fw-fixtures` builds out of the question database. The device carries
 * all of them so the setup portal's language choice has something to switch
 * to; picking between them at runtime is app_logic's job.
 *
 * When sync lands these become the fallback and the live corpus comes from
 * LittleFS.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** How many languages this image carries. At least one. */
size_t tk_corpus_count(void);

/** The two-letter code of corpus `index`, or NULL past the end. */
const char *tk_corpus_language(size_t index);

/**
 * The QDB2 bytes of corpus `index`.
 *
 * @param size receives the length
 * @return the bundle, or NULL past the end
 */
const uint8_t *tk_corpus_data(size_t index, size_t *size);

/** The index of `code`, or -1 when the image does not carry it. */
int tk_corpus_find(const char *code);

/** Where a synced corpus is kept, and what one is called. */
#define TK_CORPUS_DIR "/corpus"

#ifdef CONFIG_FILE_SYSTEM_LITTLEFS

/**
 * Read the synced corpus for `code` off the filesystem, if there is one.
 *
 * Copied into RAM rather than read in place: qdb hands out pointers into the
 * bundle and expects them to stay valid for as long as the store is open, and
 * a file's bytes are not contiguous in flash. One buffer, reused — a second
 * language replaces the first, because only one is ever open.
 *
 * @param size receives the length on success
 * @return the bundle, or NULL when there is no stored corpus for `code`, it
 *         does not fit, or it does not read back.
 */
const uint8_t *tk_corpus_stored(const char *code, size_t *size);

/**
 * Replace the stored corpus for `code`, atomically.
 *
 * Writes a staging file beside it and renames, so a power cut during a sync
 * leaves the previous corpus intact rather than half of the new one.
 *
 * @return 0, or a negative errno.
 */
int tk_corpus_store(const char *code, const uint8_t *data, size_t size);

#else

/* No filesystem in this image, so the compiled-in corpus is the only one.
 * Inline for the same reason tk_wake_button() is: callers ask
 * unconditionally and the compiler drops the branch. */
static inline const uint8_t *tk_corpus_stored(const char *code, size_t *size)
{
    (void) code;
    (void) size;

    return NULL;
}

#endif /* CONFIG_FILE_SYSTEM_LITTLEFS */

#ifdef __cplusplus
}
#endif
