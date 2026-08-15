/** The question corpora compiled into this image. */

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** How many languages this image carries. */
size_t kveld_corpus_count(void);

/** The two-letter code of corpus `index`, or NULL past the end. */
const char *kveld_corpus_language(size_t index);

/** Return corpus `index` and write its size, or return NULL past the end. */
const uint8_t *kveld_corpus_data(size_t index, size_t *size);

/** The index of `code`, or -1 when the image does not carry it. */
int kveld_corpus_find(const char *code);

/** Where a synced corpus is kept, and what one is called. */
#define KVELD_CORPUS_DIR "/corpus"

/** Room for a release version, as it appears in a manifest. */
#define KVELD_CORPUS_VERSION_LEN 32

/** The release version of the installed corpus, or an empty string. */
const char *kveld_corpus_version(void);

/** Record the installed release. Return 0 or a negative errno. */
int kveld_corpus_version_set(const char *version);

#ifdef CONFIG_FILE_SYSTEM_LITTLEFS

/** Read a synced corpus into the shared buffer, or return NULL. */
const uint8_t *kveld_corpus_stored(const char *code, size_t *size);

/** Replace the stored corpus through a staging rename. Return 0 or errno. */
int kveld_corpus_store(const char *code, const uint8_t *data, size_t size);

/** Borrow the shared corpus/download buffer and return its capacity. */
uint8_t *kveld_corpus_buffer(size_t *capacity);

#else

/** No filesystem in this image, so the compiled-in corpus is the only one. */
static inline const uint8_t *kveld_corpus_stored(const char *code, size_t *size)
{
    (void) code;
    (void) size;

    return NULL;
}

#endif /* CONFIG_FILE_SYSTEM_LITTLEFS */

#ifdef __cplusplus
}
#endif
