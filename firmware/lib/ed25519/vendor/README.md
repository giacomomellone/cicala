# TweetNaCl, vendored verbatim

Upstream: <https://tweetnacl.cr.yp.to/20140427/tweetnacl.c> and `tweetnacl.h`
from the same directory. Public domain, by Bernstein, van Gastel, Janssen,
Lange, Schwabe and Smetsers.

```
02e65bc3013ff2168983365e55906bc783c4c7e0a60d8100f17bb303a17175c4  tweetnacl.c
43f29ad721d9927b747b0100ab4160c119e7bb180c7c98a66e4bf79d31244287  tweetnacl.h
```

**Unmodified, and it should stay that way.** Verify the hashes above after any
update rather than reading the diff: the value of this file is that it is the
widely-reviewed one, and a local edit throws that away. Anything this project
needs on top of it goes in `../ed25519.cpp`.

## Why this is here

mbedtls cannot verify Ed25519. `PSA_ALG_PURE_EDDSA` is defined in
`tf-psa-crypto/include/psa/crypto_values.h` and appears nowhere in that module's
`core/` or `drivers/` — the algorithm identifier exists, the implementation does
not. What *is* vendored there is Curve25519/X25519, which is key exchange on a
Montgomery curve: a different curve form and a different operation.

TweetNaCl carries its own SHA-512, so this needs nothing from mbedtls at all.

## Why the whole file

It is the whole NaCl suite, and this project uses one function from it. Taking
only the signature path would mean editing audited crypto, which is the thing
worth avoiding; Zephyr builds with `-ffunction-sections -Wl,--gc-sections`, so
the unused half is dropped at link instead.

`randombytes()` is declared here and used only by the two key-generation
functions. This device never generates a key — it verifies against one compiled
into the image — so `../ed25519.cpp` defines it as a function that stops the
device rather than as a source of bytes that would not be random.
