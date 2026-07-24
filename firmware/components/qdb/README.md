# qdb

Question store: mounts LittleFS, parses the `.tkb` bundle format ([docs/sync_protocol.md](../../../docs/sync_protocol.md)), and serves `qdb_next(category)` with an on-flash shuffle bag (no repeats until a category is exhausted). Supports up to two installed language bundles; the active language switches in the menu. `family`-safe builds exclude questions with the spicy flag. Lands in phase C with a host-side round-trip test against `tools/build_bundle.py` output.
