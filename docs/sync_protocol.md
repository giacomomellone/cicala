# Device sync protocol and bundle format

This is the data contract for later firmware. The website hosts one signed
bundle per shipped language. A device downloads only its installed languages,
at most two.

## Manifest

Stable URL: `http://<site-domain>/device/manifest.json`

```json
{
  "schema": 3,
  "version": "2026.07.2",
  "min_fw": "0.1.0",
  "languages": {
    "en": {
      "url": "http://<site-domain>/device/bundle-en-2026.07.2.tkb2",
      "size": 15166,
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

Two things here are deliberate and are explained in the decision log.

**`url` points at the raw `.tkb2`, not the gzipped `.tkb`.** Zephyr carries no
inflate, and the saving does not justify vendoring one: about 9 KB per language
per release, on a device that is plugged in when it syncs. `size`, `sha256` and
`sig` all describe the raw bytes. `.tkb` is still published, for the website and
for people.

**`http`, not `https`.** The device has no clock, and certificate validation
without one is not validation. The signature is what protects the corpus, and it
is verified against a key compiled into the image. Because an unauthenticated
transport allows an old but validly signed manifest to be replayed, the device
**must reject a manifest whose `version` is not newer than the installed one**.

## TKB2 bundle

**TKB** is the Tischkarte Bundle: the device's copy of the question database,
in the only shape a microcontroller wants to read it. **2** is the format
version, carried in the magic bytes — version 1 existed briefly and stored one
record per question per deck, which duplicated the text.

Two file extensions appear, and they are the same data in different states:

| | What | Where |
|---|---|---|
| `.tkb` | gzip around the binary below | `dist/bundles/`, the website |
| `.tkb2` | The binary itself | what a device downloads and stores, and what the firmware suites embed |

The gzip was once the device's transport too, and `SWAP` inflated it. It no
longer is: devices fetch the `.tkb2` directly. See the decision log.

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

### Two decoders

There are now two implementations of this format, and a change to it has to
land in both:

| | Where | Used by |
|---|---|---|
| Writer + reference decoder | `tools/build_bundle.py` | the bundle build, the tools suite |
| Device reader | `firmware/lib/qdb/` | the firmware |

They are checked against each other rather than against the spec alone: the
firmware suites run the real bundles that `build_bundle.py` emits, and
`firmware/tests/qdb` additionally decodes the worked example above byte for
byte. A format change that updates only one side fails there.

The device reader is stricter than the writer needs to be — it validates every
declared length against the buffer at `open()` and rejects a bundle with
trailing bytes — because it is the side that reads files arriving over the
network. How it works is described under "qdb — the question store" in
[firmware_architecture.md](firmware_architecture.md).

## Later device flow

Normal trigger: USB power plus known Wi-Fi. Setup trigger: connect USB while
holding Next, then use the captive portal on a phone. There is no tabletop menu
or manual-sync gesture.

The portal is built and the setup trigger is not that one yet: it is both
buttons held through a boot, because VBUS detect is reserved on GPIO21 and
unwired. The sync trigger is unavailable for the same reason, so until VBUS is
wired the device syncs on a cold boot once the station has an address, and on
request from the portal's status page.

1. Fetch the manifest and check `schema`, `min_fw`, and that `version` is newer
   than the installed one.
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
