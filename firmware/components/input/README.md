# input

`app/src/input.c` reads active-low Category and Next buttons through Zephyr's
`gpio-keys` driver. Each release produces one event. A press held during boot is
reported once when released; the wake path replays the press captured by EXT1.

Category advances the retained deck. Next requests a question. Press duration
has no additional meaning. The host tests are in `firmware/tests/input/`.
