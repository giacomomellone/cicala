/* Publish a service card after the installed firmware version changes. */

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

/* `tk/fw/ver`, not `tk/fw_ver`. */
#define FIRMWARE_KEY "tk/fw/ver"

/* Last version recorded in NVS. */
static char last_seen[32];

static int firmware_load(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
    /* The settings handler receives the suffix after `tk/fw`. */
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

/* Firmware versions use their own settings subtree. */
SETTINGS_STATIC_HANDLER_DEFINE(tk_update, "tk/fw", NULL, firmware_load, NULL, NULL);

void tk_update_notice_check(void)
{
    if (strcmp(last_seen, APP_VERSION_STRING) == 0) {
        return;
    }

    /* Establish a baseline without showing an update notice. */
    const bool first_boot = last_seen[0] == '\0';

    const int err = settings_save_one(FIRMWARE_KEY, APP_VERSION_STRING, strlen(APP_VERSION_STRING));

    if (err != 0) {
        /* Store the version before publishing a one-time notice. */
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

    /* Route the notice through the app-owned service-card channel. */
    (void) zbus_chan_pub(&chan_service, &msg, K_MSEC(100));
}

#else /* !CONFIG_SETTINGS */

void tk_update_notice_check(void)
{
    /* Update notices require a persisted previous version. */
}

#endif /* CONFIG_SETTINGS */
