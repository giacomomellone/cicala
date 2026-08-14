/** Firmware updates over the air. */

#ifndef TK_OTA_H
#define TK_OTA_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum tk_ota_result {
    /** An image is in the spare slot, verified and marked for install. */
    TK_OTA_STAGED,
    /** The manifest offers nothing newer than what is running. */
    TK_OTA_CURRENT,
    /** Nothing was installed, and the running image is untouched. */
    TK_OTA_FAILED,
};

/** Check for a newer firmware release, and stage it if there is one. */
enum tk_ota_result tk_ota_run(char *version, size_t version_size);

/** The version of the image that is running, from the VERSION file. */
const char *tk_ota_running_version(void);

#ifdef __cplusplus
}
#endif

#endif /* TK_OTA_H */
