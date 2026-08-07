/*
 * Telling somebody the firmware changed.
 *
 * An update installs during a boot, which is the one moment nobody is looking:
 * MCUboot copies the image before a single line of this firmware runs, and the
 * device then comes up looking exactly as it did before. Without this the only
 * evidence an update happened at all is a console nobody has attached.
 *
 * ## Why a stored version rather than asking the bootloader
 *
 * MCUboot knows it swapped, and Zephyr exposes some of that through
 * mcuboot_swap_type(). Two things make it the wrong source. It answers "what
 * will happen next boot", not "what happened last boot", and in overwrite-only
 * mode there is nothing left afterwards to distinguish an image that was just
 * installed from one that has run a thousand times. And every wake from deep
 * sleep is a fresh boot on this device, so any answer that does not survive
 * being asked repeatedly would announce the same update over and over.
 *
 * Comparing the running version against a version in NVS answers exactly the
 * question worth asking — "is this a different firmware than the one that ran
 * here last?" — survives a flat cell, and is self-clearing: writing the new
 * value is what makes the next boot quiet. It is the same mechanism, in the
 * same settings subtree, that src/language.c uses for the corpus release.
 *
 * C rather than C++ because SETTINGS_STATIC_HANDLER_DEFINE expands to a
 * designated initializer, the same reason src/language.c is C.
 */

#include "update_notice.h"

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include <app_version.h>

#include "channels.h"

LOG_MODULE_REGISTER(tk_update, LOG_LEVEL_INF);

#ifdef CONFIG_SETTINGS

#include <zephyr/settings/settings.h>

/*
 * `tk/fw/ver`, not `tk/fw_ver`.
 *
 * The settings subsystem matches subtrees component by component, not by
 * string prefix. A handler on `tk/fw` does not match the key `tk/fw_ver` —
 * "fw" and "fw_ver" are different components — so that key falls to the `tk`
 * handler in src/language.c, which does not know it and returns -ENOENT. The
 * value saves, nothing ever loads it, and the device reports itself as
 * factory-fresh on every boot. Found on hardware, twice in a row.
 */
#define FIRMWARE_KEY "tk/fw/ver"

/** What ran here last, as far as NVS knows. Empty on a device that has never
 *  stored one, which is every device flashed before this feature existed. */
static char last_seen[32];

static int firmware_load(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
    /* What is left of the key after the `tk/fw` this handler is registered
     * for. */
    if (strcmp(name, "ver") != 0) {
        return -ENOENT;
    }

    if (len >= sizeof(last_seen)) {
        LOG_WRN("stored firmware version is %u bytes; ignoring it", (unsigned int) len);

        return -EINVAL;
    }

    const ssize_t n = read_cb(cb_arg, last_seen, len);

    if (n < 0) {
        return (int) n;
    }

    last_seen[n] = '\0';

    return 0;
}

/* A second subtree under `tk`, registered separately from src/language.c's.
 * Zephyr matches the longest registered subtree, so `tk/fw/ver` reaches this
 * handler and `tk/lang` still reaches that one. */
SETTINGS_STATIC_HANDLER_DEFINE(tk_update, "tk/fw", NULL, firmware_load, NULL, NULL);

void tk_update_notice_check(void)
{
    if (strcmp(last_seen, APP_VERSION_STRING) == 0) {
        return;
    }

    /*
     * Nothing stored: either a factory-fresh device or one whose firmware
     * predates this file. Neither is an update somebody should be told about —
     * announcing "updated to 0.1.0" to a person unboxing the thing is noise —
     * so the version is recorded silently and the next change is the first one
     * that speaks.
     */
    const bool first_boot = last_seen[0] == '\0';

    const int err = settings_save_one(FIRMWARE_KEY, APP_VERSION_STRING, strlen(APP_VERSION_STRING));

    if (err != 0) {
        /*
         * Recorded first, and this is why: a card published against a version
         * that was not stored would be shown again on every boot, forever.
         * Better to miss one announcement than to become a device that claims
         * to have just updated every time it wakes.
         */
        LOG_ERR("could not record the running firmware version: %d", err);

        return;
    }

    if (first_boot) {
        LOG_INF("firmware %s, first boot on this device", APP_VERSION_STRING);
        strcpy(last_seen, APP_VERSION_STRING);

        return;
    }

    LOG_INF("firmware updated: %s -> %s", last_seen, APP_VERSION_STRING);

    struct tk_service_msg msg = {};

    msg.len = (uint16_t) snprintk(msg.text, sizeof(msg.text),
                                  "Updated to %s. Press for a question.", APP_VERSION_STRING);

    if (msg.len >= sizeof(msg.text)) {
        msg.len = sizeof(msg.text) - 1;
    }

    strcpy(last_seen, APP_VERSION_STRING);

    /*
     * chan_service, so the card behaves like every other service card: it holds
     * the panel until somebody presses, and `app` owns the sequence number. The
     * question underneath is unchanged and comes back on that press.
     */
    (void) zbus_chan_pub(&chan_service, &msg, K_MSEC(100));
}

#else /* !CONFIG_SETTINGS */

void tk_update_notice_check(void)
{
    /*
     * Nowhere to record what ran last, so no way to know the firmware changed.
     * A board without settings storage is a host or emulator target — the
     * suites' — and one that guessed would announce an update on every single
     * boot, which is worse than saying nothing.
     *
     * Guarded rather than left out of the build, because main() calls this
     * unconditionally and src/language.c solves the same problem the same way.
     */
}

#endif /* CONFIG_SETTINGS */
