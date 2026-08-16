/** Firmware updates over the air. */

#ifndef KVELD_OTA_H
#define KVELD_OTA_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum kveld_ota_result {
    /** An image is in the spare slot, verified and marked for install. */
    KVELD_OTA_STAGED,
    /** The manifest offers nothing newer than what is running. */
    KVELD_OTA_CURRENT,
    /** Nothing was installed, and the running image is untouched. */
    KVELD_OTA_FAILED,
};

/** Check for a newer firmware release, and stage it if there is one. */
enum kveld_ota_result kveld_ota_run(char *version, size_t version_size);

/** The version of the image that is running, from the VERSION file. */
const char *kveld_ota_running_version(void);

#ifdef __cplusplus
}
#endif

#endif /* KVELD_OTA_H */
