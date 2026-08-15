/** One HTTP GET, and somewhere to put what comes back. */

#ifndef KVELD_FETCH_H
#define KVELD_FETCH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Body sink. A negative callback result aborts the transfer. */
struct kveld_fetch_sink {
    int (*write)(void *ctx, const uint8_t *data, size_t len);
    void *ctx;
};

/** Manifest and question-bundle timeout in milliseconds. */
#define KVELD_FETCH_TIMEOUT_MS 15000

/** Firmware-image timeout in milliseconds. */
#define KVELD_FETCH_IMAGE_TIMEOUT_MS 300000

/** GET `path`; return body bytes written or a negative errno. */
int kveld_fetch(const char *path, const struct kveld_fetch_sink *sink, int32_t timeout_ms);

/** Return the path inside an absolute URL, or the input when already relative. */
const char *kveld_fetch_path_of(const char *url);

/** A sink that fills a fixed buffer and refuses to overrun it. */
struct kveld_fetch_mem {
    uint8_t *buf;
    size_t capacity;
    size_t len;
    bool overflowed;
};

/** Append to `kveld_fetch_mem`, returning `-EFBIG` on overflow. */
int kveld_fetch_mem_write(void *ctx, const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* KVELD_FETCH_H */
