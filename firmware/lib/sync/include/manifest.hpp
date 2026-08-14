/* Bounded parsers for question-bundle and firmware manifests. */

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace tk
{

/** Supported question-bundle manifest schema. */
constexpr uint32_t kManifestSchema = 3;

/** Supported firmware manifest schema. */
constexpr uint32_t kFirmwareSchema = 1;

constexpr size_t kSha256Bytes = 32;
constexpr size_t kSignatureBytes = 64;
constexpr size_t kMaxUrlBytes = 160;
constexpr size_t kMaxVersionBytes = 32;

struct ManifestEntry {
    char url[kMaxUrlBytes];
    /** Bytes of the raw .qdb, checked before anything is downloaded. */
    uint32_t size;
    uint8_t sha256[kSha256Bytes];
    uint8_t sig[kSignatureBytes];
    /** False when the manifest contains `"sig": null`. */
    bool signed_;
    uint16_t count;
};

struct Manifest {
    uint32_t schema;
    char version[kMaxVersionBytes];
    char min_fw[kMaxVersionBytes];
};

/** Firmware release metadata from `firmware.json`. */
struct FirmwareRelease {
    uint32_t schema;
    char version[kMaxVersionBytes];
    char url[kMaxUrlBytes];
    uint32_t size;
    uint8_t sha256[kSha256Bytes];
    uint8_t sig[kSignatureBytes];
    /** False when the manifest contains `"sig": null`. */
    bool signed_;
};

/** Compare dotted numeric versions. Missing components count as zero. */
int version_compare(const char *a, const char *b);

/** Parse question-bundle release fields. */
bool manifest_parse(const char *json, size_t len, Manifest &out);

/** Parse one language entry. */
bool manifest_entry(const char *json, size_t len, const char *language, ManifestEntry &out);

/** Parse a firmware manifest. `"sig": null` sets `signed_` to false. */
bool firmware_parse(const char *json, size_t len, FirmwareRelease &out);

} // namespace tk
