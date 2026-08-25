/* Ed25519 verification for signed firmware and question bundles. */

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace cicala
{

constexpr size_t kEd25519SignatureBytes = 64;
constexpr size_t kEd25519PublicKeyBytes = 32;

/** Verify a detached signature. `sig` and `public_key` use the fixed sizes above. */
bool ed25519_verify(const uint8_t *sig, const uint8_t *msg, size_t msg_len,
                    const uint8_t *public_key);

} // namespace cicala
