# portal

Holding Category and Next during boot starts an open setup access point. The
portal configures Wi-Fi credentials and the question language. Captive DNS,
form parsing, page rendering, and the state machine live in `firmware/lib/portal/`;
radio and socket integration live in `firmware/app/src/`.

The panel names the access point and reports connection results. The portal
closes after its configured window. The device remains usable from compiled
question bundles without Wi-Fi. See the
[firmware architecture](../../../docs/firmware_architecture.md#the-setup-portal).
