# sync

Sync downloads the release manifest and the selected language's raw QDB4
bundle, checks its size, SHA-256 digest, and Ed25519 signature, then swaps it
into LittleFS. Any failure leaves the installed corpus unchanged.

The device uses plain HTTP because it has no trusted clock. Artifact signatures
provide authentication. Release firmware rejects unsigned manifests. External
power and explicit service requests open the sync window.

`trusted_key.h` contains the public manifest key. MCUboot uses a separate key
for firmware-image signatures. See the [sync protocol](../../../docs/sync_protocol.md)
and [firmware update guide](../../../docs/firmware_update.md).
