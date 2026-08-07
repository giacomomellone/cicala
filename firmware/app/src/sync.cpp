/*
 * Fetching a bundle, and refusing every bundle that is not exactly right.
 *
 * The flow is docs/sync_protocol.md. The order of the checks is the part worth
 * reading twice, and it is in sync.h: size, then SHA-256, then signature.
 * Cheapest first, so a truncated download costs no curve arithmetic — and
 * nothing reaches the filesystem until all three have passed.
 *
 * C++ rather than C, unlike the rest of the glue: the manifest parser and the
 * verifier are both lib/ code in namespace tk, and a C shim around them would
 * be two struct definitions that have to be kept in step by hand. Nothing here
 * needs a macro that C++ rejects — the two designated initializers this started
 * with are plain assignments now.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <psa/crypto.h>

#include "corpus.h"
#include "ed25519.hpp"
#include "fetch.h"
#include "manifest.hpp"
#include "sync.h"

extern "C" {
#include "trusted_key.h"
}

LOG_MODULE_REGISTER(tk_sync, LOG_LEVEL_INF);

/* A manifest is a few hundred bytes; this is room for every shipped language
 * several times over, and a bound on what a hostile server can make us hold. */
#define MANIFEST_MAX 2048

/*
 * The version min_fw is compared against.
 *
 * Not the image version, which lives in the VERSION file and is what OTA
 * compares — this is the version of the *sync contract* this image implements,
 * which is what min_fw is actually about. A release can require a newer
 * contract without every device that has an older image being locked out of
 * questions, and the two numbers move for different reasons.
 */
#define TK_SYNC_CONTRACT_VERSION "0.1.0"

static char manifest_buf[MANIFEST_MAX];
static size_t manifest_len;

/** GET into a caller's buffer: the manifest, and then the bundle. */
static int fetch_into(const char *path, uint8_t *into, size_t capacity)
{
    struct tk_fetch_mem mem = {};

    mem.buf = into;
    mem.capacity = capacity;

    const struct tk_fetch_sink sink = {tk_fetch_mem_write, &mem};

    const int n = tk_fetch(path, &sink, TK_FETCH_TIMEOUT_MS);

    if (n == -EFBIG && mem.overflowed) {
        LOG_ERR("GET %s returned more than the %zu bytes there was room for", path, capacity);
    }

    return n;
}

const char *tk_sync_installed_version(void)
{
    return tk_corpus_version();
}

enum tk_sync_result tk_sync_run(const char *language, uint16_t *count, char *version,
                                size_t version_size)
{
    tk::Manifest manifest = {};
    tk::ManifestEntry entry = {};

    LOG_INF("checking %s for a newer %s corpus", CONFIG_TK_SYNC_BASE_URL, language);

    manifest_len = 0;

    int n = fetch_into(tk_fetch_path_of(CONFIG_TK_SYNC_BASE_URL "/manifest.json"),
                       (uint8_t *) manifest_buf, sizeof(manifest_buf));

    if (n < 0) {
        return TK_SYNC_FAILED;
    }

    manifest_len = (size_t) n;

    if (!tk::manifest_parse(manifest_buf, manifest_len, manifest)) {
        LOG_ERR("the manifest did not parse");
        return TK_SYNC_FAILED;
    }

    if (manifest.schema != tk::kManifestSchema) {
        /* A schema this firmware does not know could mean anything, including
         * that a field it relies on now means something else. */
        LOG_ERR("manifest schema %u, expected %u", manifest.schema, tk::kManifestSchema);
        return TK_SYNC_FAILED;
    }

    if (tk::version_compare(TK_SYNC_CONTRACT_VERSION, manifest.min_fw) < 0) {
        LOG_WRN("this release wants firmware %s; this is %s", manifest.min_fw,
                TK_SYNC_CONTRACT_VERSION);
        return TK_SYNC_FAILED;
    }

    /*
     * The anti-rollback rule, and the reason the transport not being
     * authenticated is survivable: an attacker can replay an older manifest,
     * signature and all, but cannot make it look newer.
     */
    const char *installed = tk_corpus_version();

    if (installed[0] != '\0' && tk::version_compare(manifest.version, installed) <= 0) {
        LOG_INF("%s is installed and the manifest offers %s — nothing to do", installed,
                manifest.version);
        return TK_SYNC_CURRENT;
    }

    if (!tk::manifest_entry(manifest_buf, manifest_len, language, entry)) {
        LOG_ERR("the manifest carries nothing for %s", language);
        return TK_SYNC_FAILED;
    }

    if (!entry.signed_) {
        /* tools/build_bundle.py writes "sig": null without a key. A device must
         * never install one; see docs/sync_protocol.md. */
        LOG_ERR("refusing an unsigned bundle");
        return TK_SYNC_FAILED;
    }

    size_t capacity = 0;
    uint8_t *const buf = tk_corpus_buffer(&capacity);

    if (entry.size > capacity) {
        LOG_ERR("the %s bundle is %u bytes and there is room for %zu", language, entry.size,
                capacity);
        return TK_SYNC_FAILED;
    }

    LOG_INF("fetching %s (%u bytes)", entry.url, entry.size);

    n = fetch_into(tk_fetch_path_of(entry.url), buf, capacity);

    if (n < 0) {
        return TK_SYNC_FAILED;
    }

    /* First check: the cheapest one. A truncated or padded download is caught
     * before anything is hashed. */
    if ((uint32_t) n != entry.size) {
        LOG_ERR("the bundle is %d bytes; the manifest said %u", n, entry.size);
        return TK_SYNC_FAILED;
    }

    /* Second: the digest, which is what the signature actually covers.
     *
     * PSA rather than mbedtls_sha256(): mbedtls 4 moved that behind
     * mbedtls/private/, and PSA is the supported interface. It is already
     * linked — the Wi-Fi driver pulls it in. */
    uint8_t digest[tk::kSha256Bytes];
    size_t digest_len = 0;

    if (psa_crypto_init() != PSA_SUCCESS) {
        LOG_ERR("could not start the crypto subsystem");
        return TK_SYNC_FAILED;
    }

    if (psa_hash_compute(PSA_ALG_SHA_256, buf, (size_t) n, digest, sizeof(digest), &digest_len) !=
            PSA_SUCCESS ||
        digest_len != sizeof(digest)) {
        LOG_ERR("could not hash the bundle");
        return TK_SYNC_FAILED;
    }

    if (memcmp(digest, entry.sha256, sizeof(digest)) != 0) {
        LOG_ERR("the bundle does not match its digest");
        return TK_SYNC_FAILED;
    }

    /* Third: the signature, and the only one that means anything about who
     * produced the bundle. */
    if (!tk::ed25519_verify(entry.sig, digest, sizeof(digest), TISCHKARTE_TRUSTED_KEY)) {
        LOG_ERR("the bundle's signature is not valid — refusing it");
        return TK_SYNC_FAILED;
    }

    LOG_INF("bundle verified: %u questions", entry.count);

    if (tk_corpus_store(language, buf, (size_t) n) != 0) {
        return TK_SYNC_FAILED;
    }

    /* Recorded only now. A version written before the corpus was on disk would
     * make the next boot refuse the update it never received. */
    (void) tk_corpus_version_set(manifest.version);

    if (count != nullptr) {
        *count = entry.count;
    }

    if (version != nullptr && version_size > 0) {
        (void) snprintf(version, version_size, "%s", manifest.version);
    }

    return TK_SYNC_UPDATED;
}
