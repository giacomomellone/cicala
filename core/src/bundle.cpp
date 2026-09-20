#include "bundle.hpp"
#include "ed25519.hpp"

#include <string.h>

namespace cicala
{
namespace
{
bool matches(const char *bytes, size_t len, const char *text)
{
    return strlen(text) == len && !memcmp(bytes, text, len);
}
bool utf8(const char *text, size_t len)
{
    const auto *p = reinterpret_cast<const uint8_t *>(text);
    size_t i = 0;
    while (i < len) {
        uint32_t value = p[i++];
        if (value < 0x80) {
            if (value == 0 || (value < 32 && value != '\n'))
                return false;
            continue;
        }
        unsigned continuation;
        uint32_t minimum;
        if (value >= 0xc2 && value <= 0xdf) {
            continuation = 1;
            minimum = 0x80;
            value &= 0x1f;
        } else if (value >= 0xe0 && value <= 0xef) {
            continuation = 2;
            minimum = 0x800;
            value &= 0x0f;
        } else if (value >= 0xf0 && value <= 0xf4) {
            continuation = 3;
            minimum = 0x10000;
            value &= 7;
        } else
            return false;
        if (i + continuation > len)
            return false;
        while (continuation--) {
            if ((p[i] & 0xc0) != 0x80)
                return false;
            value = (value << 6) | (p[i++] & 0x3f);
        }
        if (value < minimum || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff))
            return false;
    }
    return true;
}
} // namespace

bool corpus_renderable(const Qdb &corpus)
{
    if (!corpus.is_open())
        return false;
    Question q{};
    for (uint16_t i = 0; i < corpus.count(); ++i)
        if (!corpus.at(i, q) || !q.len || q.len > kMaxQuestionBytes || !utf8(q.text, q.len))
            return false;
    return true;
}

BundleResult plan_bundle(const char *json, size_t len, const char *language, const char *installed,
                         BundlePlan &out)
{
    out = {};
    if (!json || len > kMaxManifestBytes || !language || strlen(language) > 3 || !*language ||
        !manifest_parse(json, len, out.manifest))
        return BundleResult::Invalid;
    if (out.manifest.schema != kManifestSchema ||
        version_compare(kSyncContractVersion, out.manifest.min_fw) < 0)
        return BundleResult::Incompatible;
    if (installed && *installed && version_compare(out.manifest.version, installed) <= 0)
        return BundleResult::Current;
    if (!manifest_entry(json, len, language, out.entry))
        return BundleResult::Invalid;
    if (!out.entry.signed_)
        return BundleResult::Unsigned;
    if (out.entry.size > kMaxCorpusBytes || out.entry.count > kMaxQuestions)
        return BundleResult::TooLarge;
    memcpy(out.language, language, strlen(language) + 1);
    return BundleResult::Ready;
}

BundleResult verify_bundle(const BundlePlan &plan, const uint8_t *data, size_t size,
                           const uint8_t *digest, const uint8_t *key, Qdb &out)
{
    if (!data || !digest || !key || size != plan.entry.size || size > kMaxCorpusBytes)
        return BundleResult::Invalid;
    if (!plan.entry.signed_)
        return BundleResult::Unsigned;
    if (memcmp(digest, plan.entry.sha256, kSha256Bytes))
        return BundleResult::BadDigest;
    if (!ed25519_verify(plan.entry.sig, digest, kSha256Bytes, key))
        return BundleResult::BadSignature;
    Qdb checked;
    if (!checked.open(data, size) || !corpus_renderable(checked) ||
        checked.count() != plan.entry.count ||
        !matches(checked.language(), checked.language_len(), plan.language) ||
        !matches(checked.version(), checked.version_len(), plan.manifest.version))
        return BundleResult::Invalid;
    out = checked;
    return BundleResult::Ready;
}
} // namespace cicala
