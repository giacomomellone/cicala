/** Firmware updates over the air. */

#ifndef CICALA_OTA_H
#define CICALA_OTA_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum cicala_ota_result {
    /** An image is in the spare slot, verified and marked for install. */
    CICALA_OTA_STAGED,
    /** The manifest offers nothing newer than what is running. */
    CICALA_OTA_CURRENT,
    /** Nothing was installed, and the running image is untouched. */
    CICALA_OTA_FAILED,
};

/** Check for a newer firmware release, and stage it if there is one. */
enum cicala_ota_result cicala_ota_run(char *version, size_t version_size);

/** The version of the image that is running, from the VERSION file. */
const char *cicala_ota_running_version(void);

#ifdef __cplusplus
}
#endif

#endif /* CICALA_OTA_H */
