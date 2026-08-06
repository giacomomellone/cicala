/*
 * Bundle sync: fetch a manifest, and act on it if it says something new.
 *
 * The whole flow is docs/sync_protocol.md. What this header is for is the two
 * ways it gets started and the one thing anybody else needs to know afterwards.
 *
 * ## What makes a bundle trustworthy
 *
 * Not the connection. The device has no clock, so it cannot validate a
 * certificate, and docs/decisions.md is explicit that TLS here is a transport
 * hosts accept rather than an authenticated channel. A bundle is trusted
 * because it carries an Ed25519 signature made by a key whose public half is
 * compiled into this image.
 *
 * So the order in tk_sync_run() is not arbitrary and must not be rearranged for
 * convenience: size, then SHA-256, then signature — cheapest first, so a
 * truncated download costs no curve arithmetic — and nothing is written to the
 * filesystem until all three pass.
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** How a sync ended, for the log and for the card. */
enum tk_sync_result {
    /** Nothing to do: the manifest names the release already installed. */
    TK_SYNC_CURRENT = 0,
    /** A new corpus was verified and stored. */
    TK_SYNC_UPDATED,
    /** Something went wrong. The previous corpus is untouched. */
    TK_SYNC_FAILED,
};

#ifdef CONFIG_TK_SYNC

/**
 * Check the manifest and, if it names something newer, fetch and install it.
 *
 * Blocking, and slow — a fetch, a hash and a signature check. Called from the
 * `net` thread, never from a request handler.
 *
 * Any failure leaves the installed corpus exactly as it was.
 *
 * @param language which corpus to ask about
 * @param count    receives the question count on TK_SYNC_UPDATED
 * @param version  receives the new release version on TK_SYNC_UPDATED
 */
enum tk_sync_result tk_sync_run(const char *language, uint16_t *count, char *version,
                                size_t version_size);

/** The release currently installed, or an empty string. Survives a reboot. */
const char *tk_sync_installed_version(void);

#else

static inline enum tk_sync_result tk_sync_run(const char *language, uint16_t *count, char *version,
                                              size_t version_size)
{
    (void) language;
    (void) count;
    (void) version;
    (void) version_size;

    return TK_SYNC_FAILED;
}

static inline const char *tk_sync_installed_version(void)
{
    return "";
}

#endif /* CONFIG_TK_SYNC */

#ifdef __cplusplus
}
#endif
