# Device sync protocol and bundle format

The device pulls signed per-language question bundles over HTTPS. The website deploy hosts the manifest and bundles, so the site and the device consume the same release. The device downloads **only its installed language(s)** — up to two.

## Manifest

Stable URL: `https://<site-domain>/device/manifest.json`

```json
{
  "schema": 1,
  "version": "2026.07.1",
  "min_fw": "0.1.0",
  "languages": {
    "en": {
      "url": "https://<site-domain>/device/bundle-en-2026.07.1.tkb",
      "size": 18744,
      "sha256": "9e2f…c41a",
      "sig": "base64-ed25519-signature-of-sha256",
      "count": 200
    },
    "de": { "…": "…" }
  }
}
```

- `version` is the database release (git tag `db-*`), compared per installed language.
- `sig` is an ed25519 signature **over the 32 raw bytes of the bundle's SHA-256 digest**, base64-encoded. `"sig": null` marks an unsigned dev build; release firmware rejects it.
- v2 (documented, not implemented): the manifest gains a `firmware` object for OTA; A/B partitions are already reserved.

## Bundle format `.tkb`

One bundle per shipped language, built by `tools/build_bundle.py`. A `.tkb` file is the **gzip** of the following flat binary. All integers are **little-endian**; strings are UTF-8 with no terminator.

| Field | Size | Meaning |
|---|---|---|
| magic | 4 | ASCII `TKB1` |
| version | 1 + n | u8 length, then version string |
| lang | 1 + n | u8 length, then language code |
| then, per category in the fixed order `party, family, love, work, deep`: | | |
| count | 2 | u16, number of questions in this category |
| — per question: | | |
| text | 2 + n | u16 length, then question text |
| flags | 1 | bit0 = spicy (excluded from `family`-safe builds); bits 1–7 reserved, zero |
| display | 2 | u16 display id: decimal of the first 2 id-hash bytes mod 10000. Cosmetic (`#274` on the OLED); collisions within a language are acceptable |

### Worked example

Language `en`, version `2026.07.1`, exactly one question — in `party`, id `q-8f3a2c1d`, not spicy, text `When did you last sing out loud?` (32 bytes). Display id: first two hash bytes `8f 3a` → 0x8f3a = 36666 → mod 10000 = **6666** = 0x1A0A.

Uncompressed bytes:

```
54 4B 42 31                                      "TKB1"
09 32 30 32 36 2E 30 37 2E 31                    len=9, "2026.07.1"
02 65 6E                                         len=2, "en"
01 00                                            party: count = 1
20 00                                            text length = 32
57 68 65 6E 20 64 69 64 20 79 6F 75 20 6C 61 73  "When did you las"
74 20 73 69 6E 67 20 6F 75 74 20 6C 6F 75 64 3F  "t sing out loud?"
00                                               flags = 0 (not spicy)
0A 1A                                            display = 6666 (0x1A0A LE)
00 00                                            family: count = 0
00 00                                            love:   count = 0
00 00                                            work:   count = 0
00 00                                            deep:   count = 0
```

The `.tkb` on disk is `gzip(bytes above)` (built with `mtime=0` so builds are deterministic). `tools/build_bundle.py` contains `parse_bundle()` as the executable reference decoder; the round-trip test in `tools/tests/` is the conformance check.

## Device sync flow

Trigger: on charger + known Wi-Fi, or manual "sync now" from the menu. Sync never interrupts use.

1. `GET /device/manifest.json`; check `schema` and `min_fw`.
2. For each installed language: compare `version` with the installed bundle's version. Skip if equal.
3. `GET` the bundle. Verify `size`, then SHA-256, then the ed25519 `sig` against the embedded public key (`firmware/components/sync/trusted_key.h`).
4. Write to a staging file in LittleFS, then **atomic rename** over the old bundle.
5. On next wake the OLED reports "142 new questions".

Any failure at any step: keep the old bundle, log, retry on next charge. There is no partial state — a bundle is either fully replaced or untouched.

## Keys and signing

- `tools/keygen.py` generates the ed25519 pair via the system `openssl`.
- The **private key** is a GitHub Actions secret (`BUNDLE_SIGNING_KEY`), used by the `bundle.yml` workflow on `db-*` tags. Never committed.
- The **public key** is committed at `firmware/components/sync/trusted_key.h` and baked into firmware.
- Key rotation = new firmware release; old devices keep working with their preloaded database (Wi-Fi is optional forever).
