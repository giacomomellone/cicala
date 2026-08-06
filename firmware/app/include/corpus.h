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
 * The TKB2 bytes of corpus `index`.
 *
 * @param size receives the length
 * @return the bundle, or NULL past the end
 */
const uint8_t *tk_corpus_data(size_t index, size_t *size);

/** The index of `code`, or -1 when the image does not carry it. */
int tk_corpus_find(const char *code);

#ifdef __cplusplus
}
#endif
