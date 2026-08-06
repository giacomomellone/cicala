/*
 * The `net` thread: the portal's clock, and the only place radio events turn
 * into decisions.
 *
 * It is a coordinator rather than the thread that does the networking. The HTTP
 * server has its own thread, the DHCP server runs on the socket-service thread,
 * the DNS responder has one in portal.c, and the Wi-Fi driver spawns its own.
 * What is left here is holding the state machine, translating management events
 * into its post_*() calls, and ticking it so its deadlines land.
 *
 * C rather than C++ for the one reason the rest of the glue is: the zbus
 * observer macros expand to out-of-order designated initializers.
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/sys/atomic.h>

#include "net.h"
#include "net_logic.h"
#include "portal.h"
#include "sleep.h"

LOG_MODULE_REGISTER(tk_net, LOG_LEVEL_INF);

#define NET_STACK_SIZE 6144

/*
 * Below `app` and `display`, and below the HTTP server's own thread. Nothing
 * here is on the path between a press and the panel, and a portal that answers
 * a phone a tick late costs nobody anything.
 */
#define NET_PRIORITY 8

/* While the portal runs, the machine has deadlines to notice: the scan budget
 * and the connect timeout are checked in its handlers rather than driven by an
 * event. Idle, the thread blocks until something happens. */
#define TICK_MS 250

/* Bits set by the management callback and by the HTTP handlers, drained by the
 * thread. Separate from the state machine's own flags, because these are
 * written from other threads and the machine is only ever touched by this one. */
#define EV_SCAN_DONE BIT(0)
#define EV_AP_OK BIT(1)
#define EV_AP_FAILED BIT(2)
#define EV_CONNECTED BIT(3)
#define EV_REFUSED BIT(4)
#define EV_CREDENTIALS BIT(5)

/*
 * This Zephyr has no all-events mask for Wi-Fi, so the mask is the events this
 * file actually handles. Listing them is better than a catch-all anyway: an
 * event added upstream cannot start arriving here unannounced.
 */
#define WIFI_EVENTS                                                                                \
    (NET_EVENT_WIFI_SCAN_RESULT | NET_EVENT_WIFI_SCAN_DONE | NET_EVENT_WIFI_CONNECT_RESULT |       \
     NET_EVENT_WIFI_DISCONNECT_RESULT | NET_EVENT_WIFI_AP_ENABLE_RESULT |                          \
     NET_EVENT_WIFI_AP_STA_CONNECTED)

static atomic_t events;
static K_SEM_DEFINE(wake, 0, 1);

/*
 * Set while the entry gesture is being confirmed, which is a window sleep must
 * not land in.
 *
 * The idle timer starts at the first render and runs for
 * CONFIG_TK_SLEEP_IDLE_MS; the gesture is confirmed over
 * CONFIG_TK_PORTAL_ENTRY_HOLD_MS. Both default to two seconds, so the timer
 * expires while somebody still has both buttons down. sleep_now() refuses when
 * both read closed, which covers that exact instant and nothing either side of
 * it: press the two buttons a moment apart and only one is down when the timer
 * fires, so the device sleeps armed on the other and the gesture is spent on a
 * wake instead.
 */
static atomic_t confirming_gesture;

static struct net_mgmt_event_callback wifi_cb;

static void raise(uint32_t bit)
{
    (void) atomic_or(&events, bit);
    k_sem_give(&wake);
}

bool tk_net_is_active(void)
{
    return atomic_get(&confirming_gesture) != 0 || tk_net_portal_active();
}

void tk_net_notify_credentials(void)
{
    raise(EV_CREDENTIALS);
}

/**
 * Management events, on the net_mgmt work queue.
 *
 * This runs on somebody else's thread and must not block, so it does the least
 * it can: copy a scan result, or set a bit and wake `net`.
 */
static void on_wifi_event(struct net_mgmt_event_callback *cb, uint64_t event, struct net_if *iface)
{
    ARG_UNUSED(iface);

    switch (event) {
    case NET_EVENT_WIFI_SCAN_RESULT: {
        const struct wifi_scan_result *result = (const struct wifi_scan_result *) cb->info;

        tk_portal_scan_add((const char *) result->ssid, result->ssid_length, result->rssi,
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
            tk_portal_set_station_connected(true);
            raise(EV_CONNECTED);
        } else {
            LOG_WRN("the network refused us: %d", status->status);
            raise(EV_REFUSED);
        }

        break;
    }

    case NET_EVENT_WIFI_DISCONNECT_RESULT:
        tk_portal_set_station_connected(false);
        break;

    case NET_EVENT_WIFI_AP_STA_CONNECTED:
        LOG_INF("a phone joined the setup network");
        break;

    default:
        break;
    }
}

/**
 * Were both buttons held through the boot?
 *
 * Read from the pins rather than from the input layer, which cannot answer
 * this: input.c publishes a press on release and deliberately ignores a release
 * with no press behind it, so a button already down when the device started
 * produces no event at all.
 *
 * gpio_pin_get_dt() reports the logical level, so an active-low contact reads 1
 * when it is closed — the same convention sleep.c's wake mask relies on.
 *
 * Confirmed rather than sampled once. Somebody picking the device up with two
 * fingers should not land in a service mode, and holding for two seconds is
 * hard to do by accident.
 */
static bool service_gesture_held(void)
{
    static const struct gpio_dt_spec buttons[] = {
        GPIO_DT_SPEC_GET(DT_ALIAS(tk_category), gpios),
        GPIO_DT_SPEC_GET(DT_ALIAS(tk_next), gpios),
    };

    for (int elapsed = 0; elapsed <= CONFIG_TK_PORTAL_ENTRY_HOLD_MS; elapsed += 50) {
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
        tk_net_post_scan_done();
    }

    if (pending & EV_AP_OK) {
        tk_net_post_ap_ready(true);
    }

    if (pending & EV_AP_FAILED) {
        tk_net_post_ap_ready(false);
    }

    if (pending & EV_CREDENTIALS) {
        tk_net_post_credentials();
    }

    if (pending & EV_CONNECTED) {
        tk_net_post_connected(true);
    }

    if (pending & EV_REFUSED) {
        tk_net_post_connected(false);
    }
}

static void net_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    net_mgmt_init_event_callback(&wifi_cb, on_wifi_event, WIFI_EVENTS);
    net_mgmt_add_event_callback(&wifi_cb);

    if (tk_portal_init() != 0) {
        LOG_ERR("no radio; the device runs on its compiled-in corpus");
        return;
    }

    /* Held across the gesture rather than set inside it, so the answer is
     * already true by the time the idle timer can first fire. */
    atomic_set(&confirming_gesture, 1);

    const bool gesture = !IS_ENABLED(CONFIG_TK_DEBUG_PORTAL) && service_gesture_held();

    atomic_set(&confirming_gesture, 0);

    if (IS_ENABLED(CONFIG_TK_DEBUG_PORTAL)) {
        LOG_WRN("CONFIG_TK_DEBUG_PORTAL: entering setup without the gesture");
        tk_net_post_start();
    } else if (gesture) {
        LOG_INF("both buttons held through boot — entering setup");
        tk_net_post_start();
    } else if (tk_wake_button() != TK_WAKE_NONE) {
        /*
         * A deep-sleep wake, which is to say somebody pressed a button and is
         * waiting for a question. Deep sleep makes every press a fresh boot, so
         * joining here would put a radio association in front of every question
         * the device ever answers — for a connection nothing yet uses.
         *
         * That leaves a cold boot as the only automatic trigger, and once the
         * device sleeps a cold boot is rare: first power-up, the reset pin, or
         * a flat cell. The deliberate path is the service gesture, which brings
         * the portal up and joins the saved network as the last step of its
         * flow. The charging window the design actually wants needs VBUS on
         * GPIO21, which is reserved and unwired.
         */
        LOG_INF("woken by a button; not joining a network");
    } else {
        /* A cold boot. Nothing on the panel and nothing to fetch yet: this
         * exists so the sync branch has an interface up to work with. */
        (void) tk_portal_connect_stored();
    }

    while (true) {
        /*
         * Run before waiting, as app.c does. The start posted above is a flag
         * on the machine rather than an event on the semaphore, so a loop that
         * blocked first would sit in K_FOREVER holding an unstarted portal —
         * which is exactly what the first run on hardware did.
         */
        drain_events();
        tk_net_run();

        /*
         * Blocking with no timeout when the portal is off is the same property
         * `app` has, and for the same reason: it is what lets the device reach
         * deep sleep. While the portal runs, sleep is inhibited anyway, so
         * ticking four times a second costs nothing.
         */
        (void) k_sem_take(&wake, tk_net_is_active() ? K_MSEC(TICK_MS) : K_FOREVER);
    }
}

K_THREAD_DEFINE(tk_net_thread, NET_STACK_SIZE, net_thread, NULL, NULL, NULL, NET_PRIORITY, 0, 0);
