/* Download and validate a question bundle. */

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

LOG_MODULE_REGISTER(cicala_sync, LOG_LEVEL_INF);

/* Bound manifest memory independently of the server response. */
#define MANIFEST_MAX 2048

/* The version min_fw is compared against. */
#define CICALA_SYNC_CONTRACT_VERSION "0.1.0"

static char manifest_buf[MANIFEST_MAX];
static size_t manifest_len;

/* GET into a caller's buffer: the manifest, and then the bundle. */
static int fetch_into(const char *path, uint8_t *into, size_t capacity)
{
    struct cicala_fetch_mem mem = {};

    mem.buf = into;
    mem.capacity = capacity;

    const struct cicala_fetch_sink sink = {cicala_fetch_mem_write, &mem};

    const int n = cicala_fetch(path, &sink, CICALA_FETCH_TIMEOUT_MS);

    if (n == -EFBIG && mem.overflowed) {
        LOG_ERR("GET %s returned more than the %zu bytes there was room for", path, capacity);
    }

    return n;
}

const char *cicala_sync_installed_version(void)
{
    return cicala_corpus_version();
}

enum cicala_sync_result cicala_sync_run(const char *language, uint16_t *count, char *version,
                                        size_t version_size)
{
    cicala::Manifest manifest = {};
    cicala::ManifestEntry entry = {};

    LOG_INF("checking %s for a newer %s corpus", CONFIG_CICALA_SYNC_BASE_URL, language);

    manifest_len = 0;

    int n = fetch_into(cicala_fetch_path_of(CONFIG_CICALA_SYNC_BASE_URL "/manifest.json"),
                       (uint8_t *) manifest_buf, sizeof(manifest_buf));

    if (n < 0) {
        return CICALA_SYNC_FAILED;
    }

    manifest_len = (size_t) n;

    if (!cicala::manifest_parse(manifest_buf, manifest_len, manifest)) {
        LOG_ERR("the manifest did not parse");
        return CICALA_SYNC_FAILED;
    }

    if (manifest.schema != cicala::kManifestSchema) {
        /* Unknown schemas may change field meaning. */
        LOG_ERR("manifest schema %u, expected %u", manifest.schema, cicala::kManifestSchema);
        return CICALA_SYNC_FAILED;
    }

    if (cicala::version_compare(CICALA_SYNC_CONTRACT_VERSION, manifest.min_fw) < 0) {
        LOG_WRN("this release wants firmware %s; this is %s", manifest.min_fw,
                CICALA_SYNC_CONTRACT_VERSION);
        return CICALA_SYNC_FAILED;
    }

    /* Reject signed manifests older than the installed corpus. */
    const char *installed = cicala_corpus_version();

    if (installed[0] != '\0' && cicala::version_compare(manifest.version, installed) <= 0) {
        LOG_INF("%s is installed and the manifest offers %s — nothing to do", installed,
                manifest.version);
        return CICALA_SYNC_CURRENT;
    }

    if (!cicala::manifest_entry(manifest_buf, manifest_len, language, entry)) {
        LOG_ERR("the manifest carries nothing for %s", language);
        return CICALA_SYNC_FAILED;
    }

    if (!entry.signed_) {
        /* tools/build_bundle.py writes "sig": null without a key. */
        LOG_ERR("refusing an unsigned bundle");
        return CICALA_SYNC_FAILED;
    }

    size_t capacity = 0;
    uint8_t *const buf = cicala_corpus_buffer(&capacity);

    if (entry.size > capacity) {
        LOG_ERR("the %s bundle is %u bytes and there is room for %zu", language, entry.size,
                capacity);
        return CICALA_SYNC_FAILED;
    }

    LOG_INF("fetching %s (%u bytes)", entry.url, entry.size);

    n = fetch_into(cicala_fetch_path_of(entry.url), buf, capacity);

    if (n < 0) {
        return CICALA_SYNC_FAILED;
    }

    if ((uint32_t) n != entry.size) {
        LOG_ERR("the bundle is %d bytes; the manifest said %u", n, entry.size);
        return CICALA_SYNC_FAILED;
    }

    uint8_t digest[cicala::kSha256Bytes];
    size_t digest_len = 0;

    if (psa_crypto_init() != PSA_SUCCESS) {
        LOG_ERR("could not start the crypto subsystem");
        return CICALA_SYNC_FAILED;
    }

    if (psa_hash_compute(PSA_ALG_SHA_256, buf, (size_t) n, digest, sizeof(digest), &digest_len) !=
            PSA_SUCCESS ||
        digest_len != sizeof(digest)) {
        LOG_ERR("could not hash the bundle");
        return CICALA_SYNC_FAILED;
    }

    if (memcmp(digest, entry.sha256, sizeof(digest)) != 0) {
        LOG_ERR("the bundle does not match its digest");
        return CICALA_SYNC_FAILED;
    }

    if (!cicala::ed25519_verify(entry.sig, digest, sizeof(digest), CICALA_TRUSTED_KEY)) {
        LOG_ERR("the bundle's signature is not valid — refusing it");
        return CICALA_SYNC_FAILED;
    }

    LOG_INF("bundle verified: %u questions", entry.count);

    if (cicala_corpus_store(language, buf, (size_t) n) != 0) {
        return CICALA_SYNC_FAILED;
    }

    /* Persist the version after the corpus swap succeeds. */
    (void) cicala_corpus_version_set(manifest.version);

    if (count != nullptr) {
        *count = entry.count;
    }

    if (version != nullptr && version_size > 0) {
        (void) snprintf(version, version_size, "%s", manifest.version);
    }

    return CICALA_SYNC_UPDATED;
}
