/* Network thread for portal, sync, and OTA events. */

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/zbus/zbus.h>

#include <string.h>

#include "channels.h"
#include "corpus.h"
#include "language.h"

#include "net.h"
#include "net_logic.h"
#include "ota.h"
#include "portal.h"
#include "power.h"
#include "sleep.h"
#include "status.h"
#include "sync.h"

LOG_MODULE_REGISTER(kveld_net, LOG_LEVEL_INF);

#define NET_STACK_SIZE 6144

/* Below `app` and `display`, and below the HTTP server's own thread. */
#define NET_PRIORITY 8

#define TICK_MS 250

#define EV_SCAN_DONE BIT(0)
#define EV_AP_OK BIT(1)
#define EV_AP_FAILED BIT(2)
#define EV_CONNECTED BIT(3)
#define EV_REFUSED BIT(4)
#define EV_CREDENTIALS BIT(5)
#define EV_SYNC BIT(6)

/* Wi-Fi management events handled by this module. */
#define WIFI_EVENTS                                                                                \
    (NET_EVENT_WIFI_SCAN_RESULT | NET_EVENT_WIFI_SCAN_DONE | NET_EVENT_WIFI_CONNECT_RESULT |       \
     NET_EVENT_WIFI_DISCONNECT_RESULT | NET_EVENT_WIFI_AP_ENABLE_RESULT |                          \
     NET_EVENT_WIFI_AP_STA_CONNECTED)

static atomic_t events;
static K_SEM_DEFINE(wake, 0, 1);

/* Sleep is inhibited while the entry gesture is confirmed. */
static atomic_t confirming_gesture;

static struct net_mgmt_event_callback wifi_cb;

static void run_sync(bool asked_for);
static void run_ota(void);

/* Set on a cold boot, acted on when the station reports an address. */
static bool sync_when_connected;

/* Covers station connection and the sync that follows it. */
static atomic_t sync_busy;

/* Deadline for acquiring a station address. */
static int64_t sync_deadline;

static void kveld_net_show_sync_result(enum kveld_sync_result result, uint16_t count,
                                       const char *version)
{
    struct kveld_service_msg msg = {};

    switch (result) {
    case KVELD_SYNC_UPDATED:
        msg.len = (uint16_t) snprintk(msg.text, sizeof(msg.text),
                                      "New questions: %u in this deck (%s)", count, version);
        break;

    case KVELD_SYNC_CURRENT:
        msg.len = (uint16_t) snprintk(msg.text, sizeof(msg.text), "Questions are up to date.");
        break;

    case KVELD_SYNC_FAILED:
    default:
        msg.len =
            (uint16_t) snprintk(msg.text, sizeof(msg.text), "Could not check for new questions.");
        break;
    }

    if (msg.len >= sizeof(msg.text)) {
        msg.len = sizeof(msg.text) - 1;
    }

    msg.text[msg.len] = '\0';
    kveld_portal_set_sync_result(msg.text);

    (void) zbus_chan_pub(&chan_service, &msg, K_MSEC(100));
}

static void raise(uint32_t bit)
{
    (void) atomic_or(&events, bit);
    k_sem_give(&wake);
}

void kveld_net_notify_sync(void)
{
    raise(EV_SYNC);
}

bool kveld_net_is_active(void)
{
    return atomic_get(&confirming_gesture) != 0 || atomic_get(&sync_busy) != 0 ||
           kveld_net_portal_active();
}

void kveld_net_notify_credentials(void)
{
    raise(EV_CREDENTIALS);
}

static void on_wifi_event(struct net_mgmt_event_callback *cb, uint64_t event, struct net_if *iface)
{
    ARG_UNUSED(iface);

    switch (event) {
    case NET_EVENT_WIFI_SCAN_RESULT: {
        const struct wifi_scan_result *result = (const struct wifi_scan_result *) cb->info;

        kveld_portal_scan_add((const char *) result->ssid, result->ssid_length, result->rssi,
                              result->security != WIFI_SECURITY_TYPE_NONE);
        break;
    }

    case NET_EVENT_WIFI_SCAN_DONE:
        raise(EV_SCAN_DONE);
        break;

    case NET_EVENT_WIFI_AP_ENABLE_RESULT: {
        const struct wifi_status *status = (const struct wifi_status *) cb->info;

        raise(status->status == 0 ? EV_AP_OK : EV_AP_FAILED);
        break;
    }

    case NET_EVENT_WIFI_CONNECT_RESULT: {
        const struct wifi_status *status = (const struct wifi_status *) cb->info;

        if (status->status == 0) {
            kveld_portal_clear_connection_error();
            kveld_portal_set_station_connected(true);
            raise(EV_CONNECTED);
        } else {
            LOG_WRN("the network refused us: %d", status->status);
            kveld_portal_set_connection_error(status->status);
            raise(EV_REFUSED);
        }

        break;
    }

    case NET_EVENT_WIFI_DISCONNECT_RESULT:
        kveld_portal_set_station_connected(false);
        break;

    case NET_EVENT_WIFI_AP_STA_CONNECTED:
        LOG_INF("a phone joined the setup network");
        break;

    default:
        break;
    }
}

static bool service_gesture_held(void)
{
    static const struct gpio_dt_spec buttons[] = {
        GPIO_DT_SPEC_GET(DT_ALIAS(kveld_category), gpios),
        GPIO_DT_SPEC_GET(DT_ALIAS(kveld_next), gpios),
    };

    for (int elapsed = 0; elapsed <= CONFIG_KVELD_PORTAL_ENTRY_HOLD_MS; elapsed += 50) {
        for (size_t i = 0; i < ARRAY_SIZE(buttons); i++) {
            if (gpio_pin_get_dt(&buttons[i]) != 1) {
                return false;
            }
        }

        k_sleep(K_MSEC(50));
    }

    return true;
}

static void drain_events(void)
{
    const atomic_val_t pending = atomic_clear(&events);

    if (pending & EV_SCAN_DONE) {
        kveld_net_post_scan_done();
    }

    if (pending & EV_AP_OK) {
        kveld_net_post_ap_ready(true);
    }

    if (pending & EV_AP_FAILED) {
        kveld_net_post_ap_ready(false);
    }

    if (pending & EV_CREDENTIALS) {
        kveld_net_post_credentials();
    }

    if (pending & EV_CONNECTED) {
        kveld_net_post_connected(true);

        if (sync_when_connected) {
            sync_when_connected = false;

            /* Activity and sleep inhibition use the same sync lifetime. */
            kveld_status_set_activity(true);

            run_sync(false);

            /* Questions first, firmware second. */
            if (IS_ENABLED(CONFIG_KVELD_OTA_ON_COLD_BOOT)) {
                run_ota();
            }

            kveld_status_set_activity(false);
            atomic_set(&sync_busy, 0);
        }
    }

    if (pending & EV_REFUSED) {
        kveld_net_post_connected(false);
    }

    if (pending & EV_SYNC) {
        atomic_set(&sync_busy, 1);
        kveld_status_set_activity(true);
        run_sync(true);
        run_ota();
        kveld_status_set_activity(false);
        atomic_set(&sync_busy, 0);
    }
}

static void run_sync(bool asked_for)
{
    if (!kveld_portal_station_connected()) {
        LOG_INF("no network, so nothing to sync from");
        return;
    }

    uint16_t count = 0;
    char version[KVELD_CORPUS_VERSION_LEN] = {0};

    const enum kveld_sync_result result =
        kveld_sync_run(kveld_language(), &count, version, sizeof(version));

    if (result != KVELD_SYNC_UPDATED) {
        if (asked_for) {
            kveld_net_show_sync_result(result, 0, "");
        }

        return;
    }

    /* The corpus on disk has changed, so `app` reopens it. */
    struct kveld_corpus_msg msg = {};

    (void) strncpy(msg.language, kveld_language(), sizeof(msg.language) - 1);
    (void) zbus_chan_pub(&chan_corpus, &msg, K_MSEC(100));

    kveld_net_show_sync_result(result, count, version);
}

#ifdef CONFIG_KVELD_OTA

static void run_ota(void)
{
    if (!kveld_portal_station_connected()) {
        return;
    }

    char version[32] = {0};

    if (kveld_ota_run(version, sizeof(version)) != KVELD_OTA_STAGED) {
        return;
    }

    LOG_INF("restarting to install firmware %s", version);

    /* Give deferred logs time to flush before reboot. */
    k_sleep(K_MSEC(200));

    sys_reboot(SYS_REBOOT_WARM);
}

#else

static void run_ota(void) {}

#endif /* CONFIG_KVELD_OTA */

static void net_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    net_mgmt_init_event_callback(&wifi_cb, on_wifi_event, WIFI_EVENTS);
    net_mgmt_add_event_callback(&wifi_cb);

    if (kveld_portal_init() != 0) {
        LOG_ERR("no radio; the device runs on its compiled-in corpus");
        return;
    }

    /* Inhibit sleep for the full service-entry gesture. */
    atomic_set(&confirming_gesture, 1);

    const bool gesture = !IS_ENABLED(CONFIG_KVELD_DEBUG_PORTAL) && service_gesture_held();

    atomic_set(&confirming_gesture, 0);

    if (IS_ENABLED(CONFIG_KVELD_DEBUG_PORTAL)) {
        LOG_WRN("CONFIG_KVELD_DEBUG_PORTAL: entering setup without the gesture");
        kveld_net_post_start();
    } else if (gesture) {
        LOG_INF("both buttons held through boot — entering setup");
        kveld_net_post_start();
    } else if (kveld_wake_button() != KVELD_WAKE_NONE && !kveld_power_charge_window_open()) {
        /* Battery wakes serve the button without joining Wi-Fi. */
        LOG_INF("woken by a button; not joining a network");
    } else {
        if (kveld_wake_button() != KVELD_WAKE_NONE) {
            /* A button wake during the external-power window may sync. */
            LOG_INF("woken on external power; joining to sync");
        } else {
            /* Cold boots may sync with stored credentials. */
            LOG_INF("cold boot; joining to sync");
        }

        /* Wait for the station result within the configured timeout. */
        if (kveld_portal_connect_stored()) {
            sync_when_connected = true;
            sync_deadline = k_uptime_get() + CONFIG_KVELD_NET_CONNECT_TIMEOUT_MS;
            atomic_set(&sync_busy, 1);
        }
    }

    while (true) {
        drain_events();
        kveld_net_run();

        /* A missing address must not keep the device awake indefinitely. */
        if (sync_when_connected && k_uptime_get() > sync_deadline) {
            LOG_INF("no address after %d ms; not syncing this boot",
                    CONFIG_KVELD_NET_CONNECT_TIMEOUT_MS);
            sync_when_connected = false;
            atomic_set(&sync_busy, 0);
        }

        kveld_status_set_portal(kveld_net_portal_active());

        (void) k_sem_take(&wake, kveld_net_is_active() ? K_MSEC(TICK_MS) : K_FOREVER);
    }
}

K_THREAD_DEFINE(kveld_net_thread, NET_STACK_SIZE, net_thread, NULL, NULL, NULL, NET_PRIORITY, 0, 0);
