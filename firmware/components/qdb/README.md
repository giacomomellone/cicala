# qdb

`firmware/lib/qdb/` validates QDB3 bundles and draws eligible questions without
repeats until a deck cycle is exhausted. Normal playback accepts depths 1 and
2. Dark and spicy questions remain exclusive to Wild.

Tests use bundles built from the real question database. The format is defined
in the [sync protocol](../../../docs/sync_protocol.md).
