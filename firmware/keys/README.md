# Firmware signing keys

MCUboot verifies each image with the public key compiled into the bootloader.
`SB_CONFIG_BOOT_SIGNATURE_KEY_FILE` in `firmware/app/sysbuild.conf` names the PEM
used both to generate that public key and to sign application images.

## Development key

`firmware-signing-dev.pem` is committed so local sysbuild builds produce a
matching bootloader and application without extra setup. Its private key is
public, so use it only for development hardware.

## Release key

Create the release key with:

```sh
just keygen --purpose firmware --out release-firmware-key.pem
```

Store the private PEM in the `FIRMWARE_SIGNING_KEY` GitHub Actions secret. CI
writes it over the development PEM while building a release. Never commit it.

A shipped device must receive the release-key bootloader during its wired first
flash. OTA cannot replace the bootloader or change the key it trusts. Losing or
rotating the release key requires a wired reflash of every affected device.

`BUNDLE_SIGNING_KEY` is separate. The application uses it to verify question
bundles and OTA manifests. See [firmware updates](../../docs/firmware_update.md).
