# Security

## Scope

This policy covers the code and configuration in this repository: the website, the tools that build question bundles, the firmware, the hardware design sources, and the GitHub Actions workflows. It also covers the release process: `db-*` question-database releases, `fw-*` firmware releases, and the artifacts devices download.

## Reporting a vulnerability

Use GitHub's private vulnerability reporting on this repository: **Security** tab → **Report a vulnerability**. Do not open a public issue or pull request for anything you believe is security-sensitive.

Include the component, the commit you tested, and how the issue reproduces. Please allow time for a fix and a coordinated release before disclosing publicly. Credit in the release notes is available if you want it.

## Trust model

Question bundles and firmware manifests are served over plain HTTP and verified with Ed25519 signatures against public keys compiled into the firmware. Reading this repository, cloning it, or mirroring the artifacts does not make it possible to produce an update a device accepts. A firmware update must pass both the MCUboot image signature and the manifest signature; passing one check is not enough.

The development signing key at `firmware/keys/firmware-signing-dev.pem` is committed and public by design. It is used for development hardware only. Release builds sign with keys held as GitHub Actions secrets, and shipped devices are flashed with a bootloader that trusts only the release key.

## Out of scope

- Development hardware flashed with the development key. Anyone holding the cable can already reflash it.
- Physical attacks on the device.
- Question content. Content concerns go through the issue templates.
- Availability of `cicala.dev` or the device artifact endpoint.
