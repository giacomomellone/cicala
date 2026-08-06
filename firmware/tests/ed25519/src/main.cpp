/*
 * Signature verification, against the signature that actually shipped.
 *
 * The fixture is db-2026.08.1: the SHA-256 of bundle-en-2026.08.1.qdb and the
 * ed25519 signature CI produced over it with the BUNDLE_SIGNING_KEY secret,
 * checked independently with `openssl pkeyutl -verify` before being written
 * down. The public key is the committed one, unmodified.
 *
 * So this suite answers the question that matters: does the device agree with
 * openssl about a real release? Everything else here is about what must be
 * refused, because this is the only thing standing between the corpus and
 * whoever answered the socket — the transport authenticates nothing.
 */

#include <string.h>

#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "ed25519.hpp"
#include "release_fixture.h"
#include "trusted_key.h"

using namespace tk;

namespace
{

/** A copy the mutation cases can spoil without disturbing the next one. */
uint8_t sig[kEd25519SignatureBytes];
uint8_t digest[32];
uint8_t key[kEd25519PublicKeyBytes];

void reset()
{
    memcpy(sig, kReleaseSig, sizeof(sig));
    memcpy(digest, kReleaseDigest, sizeof(digest));
    memcpy(key, TISCHKARTE_TRUSTED_KEY, sizeof(key));
}

} // namespace

ZTEST_SUITE(tk_ed25519, NULL, NULL, NULL, NULL, NULL);

ZTEST(tk_ed25519, test_the_real_release_signature_verifies)
{
    reset();

    zassert_true(ed25519_verify(sig, digest, sizeof(digest), key),
                 "the shipped db-2026.08.1 signature must verify against the committed key");
}

/*
 * A few positions rather than every bit. One verification is about a second of
 * emulated field arithmetic — far less on real silicon, but an exhaustive sweep
 * of all three inputs is several hundred of them and times the suite out. The
 * boundaries and the middle are where a wiring mistake would show: this checks
 * that a vendored implementation was hooked up correctly, not that Ed25519 is
 * sound.
 */
ZTEST(tk_ed25519, test_a_tampered_bundle_is_refused)
{
    /* The attack this exists to stop: bytes changed in flight, so the digest
     * no longer matches what was signed. */
    const size_t at[] = {0, 1, sizeof(digest) / 2, sizeof(digest) - 1};

    for (size_t i = 0; i < ARRAY_SIZE(at); i++) {
        reset();
        digest[at[i]] ^= 0x01;

        zassert_false(ed25519_verify(sig, digest, sizeof(digest), key),
                      "flipping a bit of digest byte %zu must fail", at[i]);
    }
}

ZTEST(tk_ed25519, test_a_tampered_signature_is_refused)
{
    /* Both halves of the signature: R in the first 32 bytes, S in the second. */
    const size_t at[] = {0, 31, 32, sizeof(sig) - 1};

    for (size_t i = 0; i < ARRAY_SIZE(at); i++) {
        reset();
        sig[at[i]] ^= 0x01;

        zassert_false(ed25519_verify(sig, digest, sizeof(digest), key),
                      "flipping a bit of signature byte %zu must fail", at[i]);
    }
}

ZTEST(tk_ed25519, test_another_key_cannot_sign_for_this_one)
{
    /* Somebody else's perfectly valid signature is still not ours. */
    const size_t at[] = {0, sizeof(key) / 2, sizeof(key) - 1};

    for (size_t i = 0; i < ARRAY_SIZE(at); i++) {
        reset();
        key[at[i]] ^= 0x01;

        zassert_false(ed25519_verify(sig, digest, sizeof(digest), key),
                      "flipping a bit of key byte %zu must fail", at[i]);
    }
}

ZTEST(tk_ed25519, test_an_all_zero_signature_is_refused)
{
    reset();
    memset(sig, 0, sizeof(sig));

    /* The shape a zeroed buffer or a missing field takes. It must not be
     * mistaken for a signature that happened to check out. */
    zassert_false(ed25519_verify(sig, digest, sizeof(digest), key));
}

ZTEST(tk_ed25519, test_a_truncated_or_extended_message_is_refused)
{
    reset();

    zassert_false(ed25519_verify(sig, digest, sizeof(digest) - 1, key),
                  "a shorter message is a different message");
    zassert_false(ed25519_verify(sig, digest, 0, key), "and an empty one is not a message");
}

ZTEST(tk_ed25519, test_it_refuses_rather_than_dereferences)
{
    reset();

    zassert_false(ed25519_verify(nullptr, digest, sizeof(digest), key));
    zassert_false(ed25519_verify(sig, nullptr, sizeof(digest), key));
    zassert_false(ed25519_verify(sig, digest, sizeof(digest), nullptr));

    /* Longer than the recovery buffer this bounds itself to. Refused rather
     * than overrun, which is the whole reason the bound exists. */
    zassert_false(ed25519_verify(sig, digest, 1024, key));
}
