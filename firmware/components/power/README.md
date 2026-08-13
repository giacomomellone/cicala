# power

Built. The code is `firmware/lib/power/` for what a millivolt reading means and
`firmware/app/src/power.c` for the ADC, the VBUS pin and when to look at them;
`firmware/lib/status/` and `app/src/status.c` are the two LEDs it drives. The
architecture is in `docs/firmware_architecture.md` under "Power" and the wiring
in `docs/hardware_wiring.md`.

This note predates all of that and described waking from six selector inputs,
which the two-button redesign removed. What survives from it is the constraint
that still holds: **the input and display logic must not depend on a battery
being present.** Without `CONFIG_TK_POWER` the device works exactly as it did —
it never refuses a refresh and never knows it is plugged in.

Still open, and the reason this directory is not empty: the below-30-µA
whole-device sleep target has never been measured, and cannot be on the DevKitC.
Its own power LED and its onboard WS2812 together draw one to two orders of
magnitude more than the target, so that measurement belongs to a power mule or
to rev A.
