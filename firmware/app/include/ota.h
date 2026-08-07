/*
 * Firmware updates over the air.
 *
 * The same shape as a question-bundle sync and for the same reasons: a manifest
 * says what the current release is, the device refuses anything not newer than
 * what it runs, and a signature over the artifact's SHA-256 decides whether it
 * is installed. What differs is where it goes — an image is fifty times larger
 * than the RAM there is to hold it, so it streams into the spare flash slot as
 * it arrives — and who checks it last: MCUboot verifies its own signature over
 * the image before it ever runs.
 *
 * The flow is docs/firmware_update.md.
 */

#ifndef TK_OTA_H
#define TK_OTA_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum tk_ota_result {
    /** An image is in the spare slot, verified and marked for install. The
     *  caller must reboot for it to happen; nothing has changed until then. */
    TK_OTA_STAGED,
    /** The manifest offers nothing newer than what is running. */
    TK_OTA_CURRENT,
    /** Nothing was installed, and the running image is untouched. */
    TK_OTA_FAILED,
};

/**
 * Check for a newer firmware release, and stage it if there is one.
 *
 * Blocks for as long as the download takes — hundreds of KB over Wi-Fi — so the
 * caller must be holding sleep off. Safe to call when there is no update: the
 * cost is one manifest fetch.
 *
 * @param version   filled in with the staged release's version on TK_OTA_STAGED.
 * @return what happened. Only TK_OTA_STAGED means a reboot will change anything.
 */
enum tk_ota_result tk_ota_run(char *version, size_t version_size);

/** The version of the image that is running, from the VERSION file. */
const char *tk_ota_running_version(void);

#ifdef __cplusplus
}
#endif

#endif /* TK_OTA_H */
