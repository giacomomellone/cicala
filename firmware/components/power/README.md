# power

`firmware/lib/power/` classifies battery and VBUS readings.
`firmware/app/src/power.c` samples the ADC and VBUS GPIO. `firmware/lib/status/`
and `app/src/status.c` drive the red and green LEDs.

LOW and CRITICAL states refuse e-paper refreshes. External power keeps the
device awake and opens the sync and update window. Firmware without
`CONFIG_TK_POWER` keeps refreshes enabled.

Whole-device sleep current remains unmeasured because the DevKitC indicators
exceed the 30 µA target. See [hardware wiring](../../../docs/hardware_wiring.md).
