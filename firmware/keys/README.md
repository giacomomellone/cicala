# Firmware signing keys

MCUboot refuses to install an image whose Ed25519 signature it cannot verify
against the public key compiled into it. That key comes from the PEM named by
`SB_CONFIG_BOOT_SIGNATURE_KEY_FILE` in `../app/sysbuild.conf`, and the same PEM
is what `imgtool` signs the application image with — so a bootloader and the
images it accepts are two halves of one key.

## `firmware-signing-dev.pem` — committed, and not a secret

A development key, in the tree on purpose and excepted from the `*.pem` rule in
`.gitignore`. It exists so `just fw-build` produces an image that the bootloader
from the same tree will install, with no setup step and no key to pass around.

**A device flashed with a bootloader built from this key will install any image
anyone signs with it**, and the private half is public. That is fine for a
devkit on a bench and unacceptable for anything shipped.

## The release key — never in the tree

`tools/keygen.py --purpose firmware` makes it. The private half goes into the
`FIRMWARE_SIGNING_KEY` GitHub Actions secret and nowhere else; CI writes it to
this directory under the same filename before building, so a release bootloader
embeds the release public key and trusts nothing else.

Consequences worth knowing before the first shipped device:

- Devices must be flashed over the wire with a bootloader built from the release
  key. A device carrying the dev-key bootloader cannot be upgraded into
  trusting the release key — the bootloader is what verifies, and replacing it
  is exactly what an OTA update cannot do.
- Losing the release private key means no device can ever be updated again.
- Rotating it is a wired reflash of every device.

This is a different key from `BUNDLE_SIGNING_KEY`, which signs question bundles
and the OTA manifest and is verified by the *application*. See
`docs/firmware_update.md`.
