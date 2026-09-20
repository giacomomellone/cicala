#pragma once

#include "manifest.hpp"
#include "qdb.hpp"

namespace cicala
{
enum class BundleResult {
    Ready,
    Current,
    Invalid,
    Incompatible,
    TooLarge,
    Unsigned,
    BadDigest,
    BadSignature
};
struct BundlePlan {
    Manifest manifest;
    ManifestEntry entry;
    char language[4];
};
BundleResult plan_bundle(const char *json, size_t len, const char *language, const char *installed,
                         BundlePlan &out);
/** digest must be computed over these exact payload bytes by the transport/storage adapter. */
BundleResult verify_bundle(const BundlePlan &plan, const uint8_t *data, size_t size,
                           const uint8_t digest[kSha256Bytes], const uint8_t key[32], Qdb &out);
bool corpus_renderable(const Qdb &corpus);
} // namespace cicala
