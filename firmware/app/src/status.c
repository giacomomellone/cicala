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

LOG_MODULE_REGISTER(cicala_status, LOG_LEVEL_INF);

static const struct gpio_dt_spec led_red = GPIO_DT_SPEC_GET(DT_ALIAS(cicala_led_red), gpios);
static const struct gpio_dt_spec led_green = GPIO_DT_SPEC_GET(DT_ALIAS(cicala_led_green), gpios);

/* Animation tick interval. */
#define CICALA_STATUS_TICK_MS MAX(CONFIG_CICALA_STATUS_LED_BLINK_MS / 4, 10)

static void status_tick(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(tick_work, status_tick);

/* Latest state received from other threads. */
static atomic_t pending_busy = ATOMIC_INIT(0);
static atomic_t pending_activity = ATOMIC_INIT(0);
static atomic_t pending_blocked = ATOMIC_INIT(0);
static atomic_t pending_portal = ATOMIC_INIT(0);
static atomic_t pending_portal_on = ATOMIC_INIT(0);
static atomic_t pending_power = ATOMIC_INIT(0);
static atomic_t pending_power_state = ATOMIC_INIT(CICALA_POWER_UNKNOWN);
static atomic_t stopping;
static atomic_t ready;

static void apply(uint8_t colour)
{
    const bool red = colour == CICALA_STATUS_RED || colour == CICALA_STATUS_AMBER;
    const bool green = colour == CICALA_STATUS_GREEN || colour == CICALA_STATUS_AMBER;

    (void) gpio_pin_set_dt(&led_red, red);
    (void) gpio_pin_set_dt(&led_green, green);
}

static void status_tick(struct k_work *work)
{
    ARG_UNUSED(work);

    if (atomic_get(&stopping) || !atomic_get(&ready)) {
        return;
    }

    /* Apply cross-thread events on the system workqueue. */
    if (atomic_cas(&pending_power, 1, 0)) {
        cicala_status_post_power((uint8_t) atomic_get(&pending_power_state), false);
    }
    if (atomic_cas(&pending_activity, 1, 0)) {
        cicala_status_post_activity(atomic_get(&pending_busy) != 0);
    }

    if (atomic_cas(&pending_portal, 1, 0)) {
        cicala_status_post_portal(atomic_get(&pending_portal_on) != 0);
    }

    if (atomic_cas(&pending_blocked, 1, 0)) {
        /* Use the power state already held by the arbiter. */
        cicala_status_post_power(cicala_power_state(), true);
    }

    const int64_t now = k_uptime_get();

    apply(cicala_status_output(now));

    if (cicala_status_animating(now)) {
        (void) k_work_reschedule(&tick_work, K_MSEC(CICALA_STATUS_TICK_MS));
    }
}

static void refresh(void)
{
    if (!atomic_get(&stopping)) {
        (void) k_work_reschedule(&tick_work, K_NO_WAIT);
    }
}

/* zbus listeners execute on the publisher's thread, including the ADC worker. */
static void on_power(const struct zbus_channel *chan)
{
    const struct cicala_power_msg *msg = zbus_chan_const_msg(chan);

    atomic_set(&pending_power_state, msg->state);
    atomic_set(&pending_power, 1);

    refresh();
}

ZBUS_LISTENER_DEFINE(cicala_status_obs, on_power);
ZBUS_CHAN_ADD_OBS(chan_power, cicala_status_obs, 5);

/* Called from the `net` thread. */
void cicala_status_set_activity(bool busy)
{
    atomic_set(&pending_busy, busy ? 1 : 0);
    atomic_set(&pending_activity, 1);
    refresh();
}

/* Called from the `net` thread's loop, which runs while the portal is up. */
void cicala_status_set_portal(bool on_air)
{
    atomic_set(&pending_portal_on, on_air ? 1 : 0);
    atomic_set(&pending_portal, 1);
    refresh();
}

/* Called from the `app` thread, on the press that was turned away. */
void cicala_status_note_refresh_blocked(void)
{
    atomic_set(&pending_blocked, 1);
    refresh();
}

void cicala_status_off(void)
{
    struct k_work_sync sync;
    atomic_set(&stopping, 1);
    (void) k_work_cancel_delayable_sync(&tick_work, &sync);

    if (atomic_get(&ready)) {
        apply(CICALA_STATUS_OFF);
    }
}

static int status_start(void)
{
    if (!gpio_is_ready_dt(&led_red) || !gpio_is_ready_dt(&led_green)) {
        LOG_ERR("status LEDs are not ready; running without them");
        return 0;
    }

    if (gpio_pin_configure_dt(&led_red, GPIO_OUTPUT_INACTIVE) != 0 ||
        gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_INACTIVE) != 0) {
        LOG_ERR("could not configure status LED outputs");
        return 0;
    }
    atomic_set(&ready, 1);

    /* Start dark. */
    refresh();

    return 0;
}

SYS_INIT(status_start, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
