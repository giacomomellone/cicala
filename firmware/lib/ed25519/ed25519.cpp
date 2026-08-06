#include "ed25519.hpp"

#include <string.h>

extern "C" {
#include "tweetnacl.h"
}

namespace tk
{

namespace
{

/**
 * The largest message this will verify.
 *
 * crypto_sign_open wants somewhere to put the message it recovers, and that
 * buffer has to be as large as the signed message it was given. A bundle's
 * signature covers a 32-byte digest, so this is generous at 64 — and bounding
 * it is what keeps the recovery buffer on the stack.
 */
constexpr size_t kMaxMessageBytes = 64;

} // namespace

bool ed25519_verify(const uint8_t *sig, const uint8_t *msg, size_t msg_len,
                    const uint8_t *public_key)
{
    if (sig == nullptr || msg == nullptr || public_key == nullptr) {
        return false;
    }

    if (msg_len == 0 || msg_len > kMaxMessageBytes) {
        return false;
    }

    /*
     * TweetNaCl verifies a *signed message* — the signature followed by the
     * message — rather than a detached signature, so one is assembled here.
     * The alternative would be editing the vendored file, which is the one
     * thing vendor/README.md asks not to happen.
     */
    uint8_t signed_message[kEd25519SignatureBytes + kMaxMessageBytes];
    uint8_t recovered[sizeof(signed_message)];
    unsigned long long recovered_len = 0;

    memcpy(signed_message, sig, kEd25519SignatureBytes);
    memcpy(signed_message + kEd25519SignatureBytes, msg, msg_len);

    const int err = crypto_sign_open(recovered, &recovered_len, signed_message,
                                     kEd25519SignatureBytes + msg_len, public_key);

    /*
     * Both halves matter. A non-zero return is a bad signature; a length that
     * disagrees would mean it verified something other than what was asked
     * about, which must not read as success either.
     */
    return err == 0 && recovered_len == msg_len;
}

} // namespace tk

/*
 * TweetNaCl declares this and uses it only to generate keys. This device never
 * generates one — it verifies against a key compiled into the image — so the
 * honest definition is one that cannot be mistaken for a source of entropy.
 *
 * Without it the link fails on an undefined symbol, which would be a fine
 * outcome too; this makes the reason explicit instead.
 */
extern "C" void randombytes(unsigned char *buffer, unsigned long long len)
{
    (void) buffer;
    (void) len;

    /* Reached only if something started generating keys on the device, which
     * is not a thing this firmware does. Better to stop than to sign or
     * encrypt with whatever was on the stack. */
    __builtin_trap();
}
