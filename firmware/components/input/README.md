# input

`app/src/input.c` reads active-low Filters and Next buttons through Zephyr's
`gpio-keys` driver. Each release produces one event. A press held during boot is
reported once when released; the wake path replays the press captured by EXT1.

Filters opens or advances the menu. Next draws during play, toggles a
permission in the menu, or applies Done. Press duration
has no additional meaning. The host tests are in `firmware/tests/input/`.
