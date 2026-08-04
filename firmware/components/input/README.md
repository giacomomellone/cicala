# input

**Built.** `app/src/input.c`, tested by `firmware/tests/input`.

Read six active-low, one-hot selector GPIOs and one independent active-low Next
GPIO. Inputs use pull-ups.

The selector must remain at one valid position for about 600 ms before emitting
a deck-change event. Zero or several active positions are invalid and must not
change the active deck. Next uses ordinary debounce and emits one event per
press; press duration has no additional meaning.

Per-contact debounce is the in-tree `gpio-keys` driver's, configured by
`debounce-interval-ms` in the board overlay. What is ours is the one-hot rule
and the settle window on top of it, and the boot-time read: the driver reports
only edges, so a selector already in position when power arrives would
otherwise go unnoticed — which after deep sleep is every wake.
