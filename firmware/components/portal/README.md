# portal

Holding Category and Next during boot starts a password-protected setup access
point. The e-paper service card shows the session password. The
portal configures Wi-Fi credentials and the question language, rescans nearby
networks, reports connection and update results, and can forget saved Wi-Fi.
Captive DNS, form parsing, page rendering, and the state machine live in
`firmware/lib/portal/`; radio and socket integration live in `firmware/app/src/`.

The panel names the access point, prints its session password, and reports connection results. The portal
closes after its configured window and its status page shows the remaining
window, firmware, corpus, connection, and last sync information. The device
remains usable from compiled question bundles without Wi-Fi. See the
[firmware architecture](../../../docs/firmware_architecture.md#setup-sync-and-updates).
