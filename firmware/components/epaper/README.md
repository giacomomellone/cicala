# epaper

`app/src/panel.cpp` drives a 250 × 122 GDEY0213B74 panel through Zephyr's
SSD16xx driver. `lib/layout/` handles UTF-8, accent composition, font selection,
and wrapping. The panel shows questions, deck names, and short service cards.

Full and partial refresh policy comes from measured ghosting and timing. See
[hardware wiring](../../../docs/hardware_wiring.md#bench-measurements).
