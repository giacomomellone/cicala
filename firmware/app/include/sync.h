/** Bundle sync: fetch a manifest, and act on it if it says something new. */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** How a sync ended, for the log and for the card. */
enum kveld_sync_result {
    /** Nothing to do: the manifest names the release already installed. */
    KVELD_SYNC_CURRENT = 0,
    /** A new corpus was verified and stored. */
    KVELD_SYNC_UPDATED,
    /** Something went wrong. */
    KVELD_SYNC_FAILED,
};

#ifdef CONFIG_KVELD_SYNC

/** Install a newer corpus and fill update metadata when provided. */
enum kveld_sync_result kveld_sync_run(const char *language, uint16_t *count, char *version,
                                      size_t version_size);

/** The release currently installed, or an empty string. */
const char *kveld_sync_installed_version(void);

#else

static inline enum kveld_sync_result kveld_sync_run(const char *language, uint16_t *count,
                                                    char *version, size_t version_size)
{
    (void) language;
    (void) count;
    (void) version;
    (void) version_size;

    return KVELD_SYNC_FAILED;
}

static inline const char *kveld_sync_installed_version(void)
{
    return "";
}

#endif /* CONFIG_KVELD_SYNC */

#ifdef __cplusplus
}
#endif
