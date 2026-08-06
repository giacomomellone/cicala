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
#include <zephyr/net/http/client.h>
#include <zephyr/net/socket.h>

#include <psa/crypto.h>

#include "corpus.h"
#include "ed25519.hpp"
#include "manifest.hpp"
#include "sync.h"

extern "C" {
#include "trusted_key.h"
}

LOG_MODULE_REGISTER(tk_sync, LOG_LEVEL_INF);

/* A manifest is a few hundred bytes; this is room for every shipped language
 * several times over, and a bound on what a hostile server can make us hold. */
#define MANIFEST_MAX 2048

#define HTTP_TIMEOUT_MS 15000

/*
 * The firmware version min_fw is compared against.
 *
 * Nothing else in the tree stamps one, so it is here rather than invented at
 * three call sites. It is the version of the *sync contract* this image
 * implements, which is what min_fw is actually about.
 */
#define TK_FIRMWARE_VERSION "0.1.0"

static char manifest_buf[MANIFEST_MAX];
static size_t manifest_len;

/* Where the body of whatever is being fetched goes. The manifest lands in the
 * buffer above; a bundle lands in the corpus buffer, borrowed. */
static uint8_t *body_buf;
static size_t body_cap;
static size_t body_len;
static bool body_overflowed;

static int on_body(struct http_response *rsp, enum http_final_call final, void *user_data)
{
    ARG_UNUSED(final);
    ARG_UNUSED(user_data);

    if (rsp->body_frag_len == 0) {
        return 0;
    }

    if (body_len + rsp->body_frag_len > body_cap) {
        /* Refused rather than truncated. A server that sends more than the
         * manifest promised is not one to take a prefix from. */
        body_overflowed = true;
        return 0;
    }

    memcpy(body_buf + body_len, rsp->body_frag_start, rsp->body_frag_len);
    body_len += rsp->body_frag_len;

    return 0;
}

/** Open a socket to the configured host, with TLS unless told otherwise. */
static int connect_to_host(void)
{
    struct zsock_addrinfo hints = {};
    struct zsock_addrinfo *res = nullptr;

    hints.ai_family = NET_AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    char port[8];

    (void) snprintf(port, sizeof(port), "%d", CONFIG_TK_SYNC_PORT);

    int err = zsock_getaddrinfo(CONFIG_TK_SYNC_HOST, port, &hints, &res);

    if (err != 0 || res == nullptr) {
        LOG_ERR("could not resolve %s: %d", CONFIG_TK_SYNC_HOST, err);
        return -EHOSTUNREACH;
    }

#ifdef CONFIG_TK_SYNC_INSECURE
    const int sock = zsock_socket(res->ai_family, res->ai_socktype, IPPROTO_TCP);
#else
    const int sock = zsock_socket(res->ai_family, res->ai_socktype, IPPROTO_TLS_1_2);
#endif

    if (sock < 0) {
        LOG_ERR("could not open a socket: %d", errno);
        zsock_freeaddrinfo(res);
        return -errno;
    }

#ifndef CONFIG_TK_SYNC_INSECURE
    /*
     * TLS without peer verification, deliberately and documented.
     *
     * Validating a certificate needs a trusted clock and this device has none:
     * no RTC source, no SNTP, and a wake is a fresh boot with no idea when it
     * is. Verifying with expiry checks disabled would accept a revoked or
     * expired certificate, which is most of what a certificate is for.
     *
     * So TLS here is the transport hosts will accept, plus confidentiality from
     * a passive observer. It authenticates nobody, and nothing downstream may
     * treat it as if it did — the Ed25519 signature is the whole of the
     * protection. See docs/decisions.md.
     */
    const int verify = TLS_PEER_VERIFY_NONE;

    if (zsock_setsockopt(sock, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(verify)) < 0) {
        LOG_ERR("could not set the TLS verify mode: %d", errno);
        (void) zsock_close(sock);
        zsock_freeaddrinfo(res);
        return -EIO;
    }

    /* Sent anyway: shared hosts route on it, so without it the request reaches
     * the wrong site rather than failing honestly. */
    if (zsock_setsockopt(sock, SOL_TLS, TLS_HOSTNAME, CONFIG_TK_SYNC_HOST,
                         sizeof(CONFIG_TK_SYNC_HOST)) < 0) {
        LOG_WRN("could not set the TLS hostname: %d", errno);
    }
#endif

    err = zsock_connect(sock, res->ai_addr, res->ai_addrlen);

    zsock_freeaddrinfo(res);

    if (err < 0) {
        LOG_ERR("could not connect to %s: %d", CONFIG_TK_SYNC_HOST, errno);
        (void) zsock_close(sock);
        return -ECONNREFUSED;
    }

    return sock;
}

/**
 * GET `path` into `into`, returning the number of bytes or a negative errno.
 *
 * One request per connection. Keeping one open across the manifest and the
 * bundle would save a handshake and cost a state machine for a case that
 * happens once a release.
 */
static int fetch(const char *path, uint8_t *into, size_t capacity)
{
    struct http_request req = {};
    static uint8_t recv_buf[512];

    const int sock = connect_to_host();

    if (sock < 0) {
        return sock;
    }

    body_buf = into;
    body_cap = capacity;
    body_len = 0;
    body_overflowed = false;

    req.method = HTTP_GET;
    req.url = path;
    req.host = CONFIG_TK_SYNC_HOST;
    req.protocol = "HTTP/1.1";
    req.response = on_body;
    req.recv_buf = recv_buf;
    req.recv_buf_len = sizeof(recv_buf);

    const int err = http_client_req(sock, &req, HTTP_TIMEOUT_MS, nullptr);

    (void) zsock_close(sock);

    if (err < 0) {
        LOG_ERR("GET %s failed: %d", path, err);
        return err;
    }

    if (body_overflowed) {
        LOG_ERR("GET %s returned more than the %zu bytes there was room for", path, capacity);
        return -EFBIG;
    }

    if (body_len == 0) {
        LOG_ERR("GET %s returned nothing", path);
        return -ENODATA;
    }

    return (int) body_len;
}

/** The path part of a URL, which is all http_client_req wants. */
static const char *path_of(const char *url)
{
    const char *at = strstr(url, "://");

    if (at == nullptr) {
        return url;
    }

    at = strchr(at + 3, '/');

    return at != nullptr ? at : "/";
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

    int n = fetch(path_of(CONFIG_TK_SYNC_BASE_URL "/manifest.json"), (uint8_t *) manifest_buf,
                  sizeof(manifest_buf));

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

    if (tk::version_compare(TK_FIRMWARE_VERSION, manifest.min_fw) < 0) {
        LOG_WRN("this release wants firmware %s; this is %s", manifest.min_fw, TK_FIRMWARE_VERSION);
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

    n = fetch(path_of(entry.url), buf, capacity);

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
