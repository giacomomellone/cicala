/*
 * One HTTP GET, and somewhere to put what comes back.
 *
 * Shared by the two things that download: sync.cpp, which fetches a manifest
 * and a question bundle into RAM, and ota.cpp, which fetches a firmware image
 * that is fifty times too large to hold. The difference between them is the
 * sink, so the sink is the parameter.
 *
 * Nothing here authenticates anybody. What arrives is bytes from a host that
 * the device cannot verify — it has no clock, so it cannot validate a
 * certificate — and every caller is expected to check a signature over what it
 * received before acting on it. See docs/decisions.md.
 */

#ifndef TK_FETCH_H
#define TK_FETCH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Where the body goes, a chunk at a time.
 *
 * Called from the HTTP client's response callback, so it runs on the calling
 * thread and may block — writing to flash is a legitimate thing to do here.
 *
 * @return 0 to accept the chunk. Anything negative aborts the transfer, and
 *         tk_fetch() returns that value.
 */
struct tk_fetch_sink {
    int (*write)(void *ctx, const uint8_t *data, size_t len);
    void *ctx;
};

/**
 * GET `path` from the configured sync host into `sink`.
 *
 * One request per connection: keeping one open across the manifest and what it
 * points at would save a handshake and cost a state machine, for a case that
 * happens once a release.
 *
 * @return the number of body bytes delivered to the sink, or a negative errno.
 */
int tk_fetch(const char *path, const struct tk_fetch_sink *sink);

/**
 * The path part of a URL, which is all the HTTP client wants.
 *
 * A manifest names absolute URLs so it reads the same to a person and to a
 * browser, but the host is a build-time setting and the socket is already open
 * to it by the time this is called.
 */
const char *tk_fetch_path_of(const char *url);

/** A sink that fills a fixed buffer and refuses to overrun it. */
struct tk_fetch_mem {
    uint8_t *buf;
    size_t capacity;
    size_t len;
    bool overflowed;
};

/** The write function for `struct tk_fetch_mem`. */
int tk_fetch_mem_write(void *ctx, const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* TK_FETCH_H */
