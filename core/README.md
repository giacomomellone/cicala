# Cicala portable core

Allocation-free C++17 question selection, session transactions, signed QDB4
verification, and SD snapshot serialization. Zephyr builds the same sources
through `firmware/lib/` CMake adapters; the CrossPoint fork consumes an immutable
package. See [the architecture](../docs/crosspoint.md).

```sh
cmake -S core -B build/core -DCICALA_BUILD_TESTS=ON
cmake --build build/core --parallel
ctest --test-dir build/core --output-on-failure
just core-package
```

Host tests need CMake, a C++17 compiler, and Python 3. The bundle roundtrip test
also needs OpenSSL with Ed25519 support. On macOS install `openssl@3`, or set
`OPENSSL` to its executable. The package has CMake and PlatformIO entry points.

`config.hpp` defines bounded defaults. Every translation unit must use the same
`CICALA_MAX_QUESTIONS`, `CICALA_RECENT_RING`, `CICALA_MAX_QUESTION_BYTES`, and
`CICALA_TEXTURE` definitions. Zephyr maps its Kconfig values to these macros.
Capacity changes invalidate SD snapshots. Changing the RTC layout also requires
the Zephyr retained-block migration rules.

`Qdb` borrows its input bytes. A `Session` borrows its committed `PlayState`,
`Bag::State`, and `Qdb`; their lifetimes must cover use of the session. Cancel
pending work before replacing the corpus. The prepared view owns a bounded
copy of the question. Only `complete(token, RenderResult::Complete)` commits it.
Calls on a session must be serialized by its adapter.

`Action::CancelFilters` discards an open filter draft and returns to the committed
question or empty view without drawing from the bag. Cancellation follows the same
render-commit protocol as Apply; a failed refresh retains the open draft.

`verify_bundle` takes a SHA-256 digest computed by the platform over the exact
payload passed with it. It verifies that digest's signature and validates the
payload against its manifest. The adapter owns streaming, hashing, storage,
trust-key provisioning, cancellation, and activation.

Code is MIT; question assets are CC0. TweetNaCl's attribution is retained in
`src/vendor/README.md`. The packaged English firmware fixture is for tests and
is not the offline product deck.
