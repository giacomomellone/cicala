/*
 * The sync manifest, and the rules for deciding whether to act on one.
 *
 * No Zephyr headers, so the suite runs real manifests on the host.
 *
 * ## Why this parses JSON by hand
 *
 * The manifest is small, fixed in shape, and has one awkward property: the
 * language keys are data. `languages` is an object whose members are `en`, `de`
 * and whatever ships later, which is exactly what Zephyr's descriptor-driven
 * JSON parser cannot describe. Extracting the handful of fields wanted here is
 * less code than working around that, and it is code a test can drive with a
 * malformed manifest — which matters, because this parses bytes from a
 * transport that authenticates nobody.
 *
 * Nothing here trusts what it reads. The parser's job is to answer "does this
 * say a newer bundle exists, and where", and every field it fills in is bounded
 * before it is written.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace tk
{

/** The manifest shape this firmware understands. See docs/sync_protocol.md. */
constexpr uint32_t kManifestSchema = 3;

constexpr size_t kSha256Bytes = 32;
constexpr size_t kSignatureBytes = 64;
constexpr size_t kMaxUrlBytes = 160;
constexpr size_t kMaxVersionBytes = 32;

/** What the manifest says about one language. */
struct ManifestEntry {
    char url[kMaxUrlBytes];
    /** Bytes of the raw .qdb, checked before anything is downloaded. */
    uint32_t size;
    uint8_t sha256[kSha256Bytes];
    uint8_t sig[kSignatureBytes];
    /** False for a development bundle built without a key. Release firmware
     *  refuses these; see docs/sync_protocol.md. */
    bool signed_;
    uint16_t count;
};

/** What it says about the release as a whole. */
struct Manifest {
    uint32_t schema;
    char version[kMaxVersionBytes];
    char min_fw[kMaxVersionBytes];
};

/**
 * Compare two dotted version strings component by component, numerically.
 *
 * `2026.08.9` is older than `2026.08.10`, which string comparison gets exactly
 * backwards — and this decides whether an update is applied, so getting it
 * backwards would mean accepting a rollback. Missing components count as zero,
 * so `2026.08` is older than `2026.08.1`.
 *
 * @return <0 when `a` is older, 0 when equal, >0 when `a` is newer.
 */
int version_compare(const char *a, const char *b);

/** Read the release-level fields. False when the manifest is not usable. */
bool manifest_parse(const char *json, size_t len, Manifest &out);

/**
 * Read one language's entry.
 *
 * @return false when the manifest carries nothing for `language`, or when what
 *         it carries does not fit or does not decode.
 */
bool manifest_entry(const char *json, size_t len, const char *language, ManifestEntry &out);

} // namespace tk
