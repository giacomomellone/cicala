# input

Read six active-low, one-hot selector GPIOs and one independent active-low Next
GPIO. Inputs use pull-ups.

The selector must remain at one valid position for about 600 ms before emitting
a deck-change event. Zero or several active positions are invalid and must not
change the active deck. Next uses ordinary debounce and emits one event per
press; press duration has no additional meaning.
