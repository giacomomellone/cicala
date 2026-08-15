#include "ed25519.hpp"

#include <string.h>

extern "C" {
#include "tweetnacl.h"
}

namespace kveld
{

namespace
{

/** Maximum message size accepted by the fixed stack buffers. */
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

    // TweetNaCl expects the detached signature prepended to the message.
    uint8_t signed_message[kEd25519SignatureBytes + kMaxMessageBytes];
    uint8_t recovered[sizeof(signed_message)];
    unsigned long long recovered_len = 0;

    memcpy(signed_message, sig, kEd25519SignatureBytes);
    memcpy(signed_message + kEd25519SignatureBytes, msg, msg_len);

    const int err = crypto_sign_open(recovered, &recovered_len, signed_message,
                                     kEd25519SignatureBytes + msg_len, public_key);

    return err == 0 && recovered_len == msg_len;
}

} // namespace kveld

// TweetNaCl requires this symbol for key generation, which firmware forbids.
extern "C" void randombytes(unsigned char *buffer, unsigned long long len)
{
    (void) buffer;
    (void) len;

    __builtin_trap();
}
