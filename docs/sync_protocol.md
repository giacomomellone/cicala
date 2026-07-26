# Device sync protocol and bundle format

This is the data contract for later firmware. The website hosts one signed
bundle per shipped language. A device downloads only its installed languages,
at most two.

## Manifest

Stable URL: `https://<site-domain>/device/manifest.json`

```json
{
  "schema": 2,
  "version": "2026.07.2",
  "min_fw": "0.1.0",
  "languages": {
    "en": {
      "url": "https://<site-domain>/device/bundle-en-2026.07.2.tkb",
      "size": 19042,
      "sha256": "9e2f…c41a",
      "sig": "base64-ed25519-signature-of-sha256",
      "count": 240
    }
  }
}
```

`count` is the number of unique stored questions, not the sum of deck
eligibilities. `sig` is an Ed25519 signature over the 32 raw bytes of the
bundle's SHA-256 digest. An unsigned development bundle uses `"sig": null`;
release firmware rejects it.

## TKB2 bundle

A `.tkb` file is a deterministic gzip stream (`mtime=0`) around this flat
binary. Integers are little-endian and strings are UTF-8 without a terminator.

| Field | Size | Meaning |
|---|---:|---|
| magic | 4 | ASCII `TKB2` |
| version | 1 + n | u8 length, then release version |
| language | 1 + n | u8 length, then language code |
| count | 2 | u16 number of unique questions |
| then, per question | | |
| deck mask | 1 | bits 0–5 are `new_people`, `close`, `family`, `work`, `here`, `wild`; bits 6–7 are zero |
| metadata | 1 | bits 0–1 are `depth - 1`; bit 2 is `spicy`; bit 3 is `dark`; bits 4–7 are zero |
| text | 2 + n | u16 byte length, then question text |

The bundle omits IDs because the physical device has no favorites, permalinks,
or human-visible question numbers. IDs remain in the repository and website.
A question with several eligible decks is stored once with several mask bits,
which avoids duplicated text.

Dark and spicy flags are tone metadata. Corpus validation requires either flag
to be exclusive to the Wild deck. Normal playback filters out depth 3.

### Worked example

Language `en`, version `2026.07.2`, with one depth-2 question eligible for New
People and Close:

`When did you last sing out loud?`

Its deck mask is `00000011` and metadata is `00000001`.

```text
54 4B 42 32                                      "TKB2"
09 32 30 32 36 2E 30 37 2E 32                    len=9, "2026.07.2"
02 65 6E                                         len=2, "en"
01 00                                            count = 1
03                                               new_people + close
01                                               depth 2, no tone flags
20 00                                            text length = 32
57 68 65 6E 20 64 69 64 20 79 6F 75 20 6C 61 73
74 20 73 69 6E 67 20 6F 75 74 20 6C 6F 75 64 3F
```

`tools/build_bundle.py` contains `parse_bundle()`, the executable reference
decoder. The tools unit suite round-trips both shipped languages.

## Later device flow

Normal trigger: USB power plus known Wi-Fi. Setup trigger: connect USB while
holding Next, then use the captive portal on a phone. There is no tabletop menu
or manual-sync gesture.

1. Fetch the manifest and check `schema` and `min_fw`.
2. Compare the release with each installed language.
3. Download a changed bundle and verify size, SHA-256, then signature.
4. Write a staging file and atomically rename it over the prior bundle.
5. Use the new bundle on the next requested draw without displaying a status
   message.

Any failure leaves the old bundle intact and retries during a later charging
window. Sync does not refresh the e-paper or take attention from the current
question.

## Keys

`tools/keygen.py` generates the Ed25519 pair through the system `openssl`.
The private key is a GitHub Actions secret. The public key remains part of a
future firmware build. Key rotation requires a firmware release; devices
without a valid new bundle continue with their preloaded corpus.
