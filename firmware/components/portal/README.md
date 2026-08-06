# portal

**Built.** `firmware/lib/portal/` holds the parts that need no radio — the
setup state machine, the DNS responder's codec, the form decoder and the pages
— and `firmware/app/src/{net.c,net_logic.cpp,portal.c}` hold the thread, the
SoftAP and the sockets. Tested by `firmware/tests/portal`. How it works is
described under "The setup portal" in
[docs/firmware_architecture.md](../../../docs/firmware_architecture.md); this
file is the contract it was built to.

Optional service-only SoftAP and captive portal for Wi-Fi credentials and
language selection. It is entered through a documented service gesture, never
through a tabletop display menu.

The device remains fully usable with factory-preloaded questions when Wi-Fi is
never configured.

Two departures from the contract as written, both in the decision log: the
gesture is both buttons held through a boot rather than USB-plus-Next, because
VBUS detect is unwired; and the portal does put one card on the e-paper, saying
which network to join, because a phone cannot join a network nobody has named.
