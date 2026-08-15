/* Status LED output and animation. */

#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/zbus/zbus.h>

#include "channels.h"
#include "power_logic.h"
#include "status.h"
#include "status_logic.h"

LOG_MODULE_REGISTER(kveld_status, LOG_LEVEL_INF);

static const struct gpio_dt_spec led_red = GPIO_DT_SPEC_GET(DT_ALIAS(kveld_led_red), gpios);
static const struct gpio_dt_spec led_green = GPIO_DT_SPEC_GET(DT_ALIAS(kveld_led_green), gpios);

/* Animation tick interval. */
#define KVELD_STATUS_TICK_MS MAX(CONFIG_KVELD_STATUS_LED_BLINK_MS / 4, 10)

static void status_tick(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(tick_work, status_tick);

/* Latest state received from other threads. */
static atomic_t pending_busy = ATOMIC_INIT(0);
static atomic_t pending_activity = ATOMIC_INIT(0);
static atomic_t pending_blocked = ATOMIC_INIT(0);
static atomic_t pending_portal = ATOMIC_INIT(0);
static atomic_t pending_portal_on = ATOMIC_INIT(0);

static void apply(uint8_t colour)
{
    const bool red = colour == KVELD_STATUS_RED || colour == KVELD_STATUS_AMBER;
    const bool green = colour == KVELD_STATUS_GREEN || colour == KVELD_STATUS_AMBER;

    (void) gpio_pin_set_dt(&led_red, red);
    (void) gpio_pin_set_dt(&led_green, green);
}

static void status_tick(struct k_work *work)
{
    ARG_UNUSED(work);

    /* Apply cross-thread events on the system workqueue. */
    if (atomic_cas(&pending_activity, 1, 0)) {
        kveld_status_post_activity(atomic_get(&pending_busy) != 0);
    }

    if (atomic_cas(&pending_portal, 1, 0)) {
        kveld_status_post_portal(atomic_get(&pending_portal_on) != 0);
    }

    if (atomic_cas(&pending_blocked, 1, 0)) {
        /* Use the power state already held by the arbiter. */
        kveld_status_post_power(kveld_power_state(), true);
    }

    const int64_t now = k_uptime_get();

    apply(kveld_status_output(now));

    if (kveld_status_animating(now)) {
        (void) k_work_reschedule(&tick_work, K_MSEC(KVELD_STATUS_TICK_MS));
    }
}

static void refresh(void)
{
    (void) k_work_reschedule(&tick_work, K_NO_WAIT);
}

/* Power updates are serialized through the system workqueue. */
static void on_power(const struct zbus_channel *chan)
{
    const struct kveld_power_msg *msg = zbus_chan_const_msg(chan);

    kveld_status_post_power(msg->state, false);

    refresh();
}

ZBUS_LISTENER_DEFINE(kveld_status_obs, on_power);
ZBUS_CHAN_ADD_OBS(chan_power, kveld_status_obs, 5);

/* Called from the `net` thread. */
void kveld_status_set_activity(bool busy)
{
    atomic_set(&pending_busy, busy ? 1 : 0);
    atomic_set(&pending_activity, 1);
    refresh();
}

/* Called from the `net` thread's loop, which runs while the portal is up. */
void kveld_status_set_portal(bool on_air)
{
    atomic_set(&pending_portal_on, on_air ? 1 : 0);
    atomic_set(&pending_portal, 1);
    refresh();
}

/* Called from the `app` thread, on the press that was turned away. */
void kveld_status_note_refresh_blocked(void)
{
    atomic_set(&pending_blocked, 1);
    refresh();
}

void kveld_status_off(void)
{
    (void) k_work_cancel_delayable(&tick_work);

    apply(KVELD_STATUS_OFF);
}

static int status_start(void)
{
    if (!gpio_is_ready_dt(&led_red) || !gpio_is_ready_dt(&led_green)) {
        LOG_ERR("status LEDs are not ready; running without them");
        return 0;
    }

    (void) gpio_pin_configure_dt(&led_red, GPIO_OUTPUT_INACTIVE);
    (void) gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_INACTIVE);

    /* Start dark. */
    refresh();

    return 0;
}

SYS_INIT(status_start, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
