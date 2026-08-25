/** One HTTP GET, and somewhere to put what comes back. */

#ifndef CICALA_FETCH_H
#define CICALA_FETCH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Body sink. A negative callback result aborts the transfer. */
struct cicala_fetch_sink {
    int (*write)(void *ctx, const uint8_t *data, size_t len);
    void *ctx;
};

/** Manifest and question-bundle timeout in milliseconds. */
#define CICALA_FETCH_TIMEOUT_MS 15000

/** Firmware-image timeout in milliseconds. */
#define CICALA_FETCH_IMAGE_TIMEOUT_MS 300000

/** GET `path`; return body bytes written or a negative errno. */
int cicala_fetch(const char *path, const struct cicala_fetch_sink *sink, int32_t timeout_ms);

/** Return the path inside an absolute URL, or the input when already relative. */
const char *cicala_fetch_path_of(const char *url);

/** A sink that fills a fixed buffer and refuses to overrun it. */
struct cicala_fetch_mem {
    uint8_t *buf;
    size_t capacity;
    size_t len;
    bool overflowed;
};

/** Append to `cicala_fetch_mem`, returning `-EFBIG` on overflow. */
int cicala_fetch_mem_write(void *ctx, const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* CICALA_FETCH_H */
