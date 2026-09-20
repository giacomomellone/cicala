/* Download and validate a question bundle. */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <psa/crypto.h>

#include "corpus.h"
#include "bundle.hpp"
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
    static cicala::BundlePlan plan;
    const auto &manifest = plan.manifest;
    const auto &entry = plan.entry;

    LOG_INF("checking %s for a newer %s corpus", CONFIG_CICALA_SYNC_BASE_URL, language);

    manifest_len = 0;

    int n = fetch_into(cicala_fetch_path_of(CONFIG_CICALA_SYNC_BASE_URL "/manifest.json"),
                       (uint8_t *) manifest_buf, sizeof(manifest_buf));

    if (n < 0) {
        return CICALA_SYNC_FAILED;
    }

    manifest_len = (size_t) n;

    const auto planned =
        cicala::plan_bundle(manifest_buf, manifest_len, language, cicala_corpus_version(), plan);
    if (planned == cicala::BundleResult::Current)
        return CICALA_SYNC_CURRENT;
    if (planned != cicala::BundleResult::Ready) {
        LOG_ERR("bundle manifest rejected (%d)", static_cast<int>(planned));
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

    cicala::Qdb checked;
    if (cicala::verify_bundle(plan, buf, static_cast<size_t>(n), digest, CICALA_TRUSTED_KEY,
                              checked) != cicala::BundleResult::Ready) {
        LOG_ERR("bundle verification failed");
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
