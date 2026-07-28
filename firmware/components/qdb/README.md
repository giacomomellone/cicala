# qdb

Mount the question storage, parse the TKB2 bundle format described in
[sync_protocol.md](../../../docs/sync_protocol.md), and provide the next
eligible question for a selected deck without repeats until that deck's shuffle
bag is exhausted.

Normal playback excludes depth 3. Dark and spicy questions remain exclusive to
the Wild deck. Host-side tests must round-trip real bundles produced by
`tools/build_bundle.py`.
