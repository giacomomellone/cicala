
#include <string.h>

#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "ed25519.hpp"
#include "release_fixture.h"
#include "trusted_key.h"

using namespace cicala;

namespace
{

uint8_t sig[kEd25519SignatureBytes];
uint8_t digest[32];
uint8_t key[kEd25519PublicKeyBytes];

void reset()
{
    memcpy(sig, kReleaseSig, sizeof(sig));
    memcpy(digest, kReleaseDigest, sizeof(digest));
    memcpy(key, CICALA_TRUSTED_KEY, sizeof(key));
}

} // namespace

ZTEST_SUITE(cicala_ed25519, NULL, NULL, NULL, NULL, NULL);

ZTEST(cicala_ed25519, test_the_manifest_signature_verifies)
{
    reset();

    zassert_true(ed25519_verify(sig, digest, sizeof(digest), key),
                 "the firmware manifest signature must verify against the committed key");
}

ZTEST(cicala_ed25519, test_a_tampered_digest_is_refused)
{
    const size_t at[] = {0, 1, sizeof(digest) / 2, sizeof(digest) - 1};

    for (size_t i = 0; i < ARRAY_SIZE(at); i++) {
        reset();
        digest[at[i]] ^= 0x01;

        zassert_false(ed25519_verify(sig, digest, sizeof(digest), key),
                      "flipping a bit of digest byte %zu must fail", at[i]);
    }
}

ZTEST(cicala_ed25519, test_a_tampered_signature_is_refused)
{
    const size_t at[] = {0, 31, 32, sizeof(sig) - 1};

    for (size_t i = 0; i < ARRAY_SIZE(at); i++) {
        reset();
        sig[at[i]] ^= 0x01;

        zassert_false(ed25519_verify(sig, digest, sizeof(digest), key),
                      "flipping a bit of signature byte %zu must fail", at[i]);
    }
}

ZTEST(cicala_ed25519, test_another_key_cannot_sign_for_this_one)
{
    const size_t at[] = {0, sizeof(key) / 2, sizeof(key) - 1};

    for (size_t i = 0; i < ARRAY_SIZE(at); i++) {
        reset();
        key[at[i]] ^= 0x01;

        zassert_false(ed25519_verify(sig, digest, sizeof(digest), key),
                      "flipping a bit of key byte %zu must fail", at[i]);
    }
}

ZTEST(cicala_ed25519, test_an_all_zero_signature_is_refused)
{
    reset();
    memset(sig, 0, sizeof(sig));

    zassert_false(ed25519_verify(sig, digest, sizeof(digest), key));
}

ZTEST(cicala_ed25519, test_a_truncated_or_extended_message_is_refused)
{
    reset();

    zassert_false(ed25519_verify(sig, digest, sizeof(digest) - 1, key),
                  "a shorter message is a different message");
    zassert_false(ed25519_verify(sig, digest, 0, key), "and an empty one is not a message");
}

ZTEST(cicala_ed25519, test_it_refuses_rather_than_dereferences)
{
    reset();

    zassert_false(ed25519_verify(nullptr, digest, sizeof(digest), key));
    zassert_false(ed25519_verify(sig, nullptr, sizeof(digest), key));
    zassert_false(ed25519_verify(sig, digest, sizeof(digest), nullptr));

    zassert_false(ed25519_verify(sig, digest, 1024, key));
}
