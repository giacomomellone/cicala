/*
 * Ed25519 signature verification, and nothing else.
 *
 * No Zephyr headers, so the suite runs real release signatures on the host.
 *
 * This is what protects the question corpus. The transport does not: the device
 * has no clock, so it cannot validate a TLS certificate, and
 * docs/decisions.md is explicit that TLS here is a transport hosts accept
 * rather than an authenticated channel. A bundle is trusted because it carries
 * a signature made by a key whose public half is compiled into this image — not
 * because of who answered the socket.
 *
 * So the rule for anything downstream: **a bundle that does not verify is not a
 * bundle**. There is no path that skips this because the connection "looked
 * fine", and there is no unsigned mode in a shipped build.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace tk
{

/** Ed25519 sizes, fixed by the algorithm. */
constexpr size_t kEd25519SignatureBytes = 64;
constexpr size_t kEd25519PublicKeyBytes = 32;

/**
 * Does `sig` sign `msg` under `public_key`?
 *
 * Constant in what it reveals: it answers yes or no and never says which byte
 * disagreed.
 *
 * @param sig         kEd25519SignatureBytes of detached signature
 * @param msg         the signed message — for a bundle, the 32 raw bytes of
 *                    its SHA-256, which is what tools/build_bundle.py signs
 * @param public_key  kEd25519PublicKeyBytes, from trusted_key.h
 * @return true only when the signature is valid.
 */
bool ed25519_verify(const uint8_t *sig, const uint8_t *msg, size_t msg_len,
                    const uint8_t *public_key);

} // namespace tk
