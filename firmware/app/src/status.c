/*
 * Two pins, and when to change them.
 *
 * Compiled only when CONFIG_TK_STATUS_LED is on, which needs a board with a
 * tk_leds devicetree node. What each condition should look like is lib/status's
 * job, reached through status_logic.h; this file owns the GPIOs and the timing.
 *
 * ## Why a work item rather than a thread
 *
 * The longest thing this ever does is set two pins. It runs on the system
 * workqueue alongside the ADC sampling, and reschedules itself only while
 * something is actually moving — a steady colour asks for nothing, which is the
 * ordinary case and the one that must not keep the device out of deep sleep.
 *
 * ## Why the LEDs are not a reason to stay awake
 *
 * They are not, deliberately. Nothing here inhibits sleep. On battery the
 * device is awake for about two seconds after a press and then stops existing,
 * and that is exactly the right amount of LED for a device running on a cell:
 * the blink answering a refused press fits inside it, and a steady colour
 * nobody is looking at does not outlive it. The LEDs stay lit through a whole
 * charge because `power` holds the device awake on external power, not because
 * this file asked for anything.
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "channels.h"
#include "net.h"
#include "power_logic.h"
#include "status.h"
#include "status_logic.h"

LOG_MODULE_REGISTER(tk_status, LOG_LEVEL_INF);

static const struct gpio_dt_spec led_red = GPIO_DT_SPEC_GET(DT_ALIAS(tk_led_red), gpios);
static const struct gpio_dt_spec led_green = GPIO_DT_SPEC_GET(DT_ALIAS(tk_led_green), gpios);

/*
 * How often to look while something is moving. A blink is
 * CONFIG_TK_STATUS_LED_BLINK_MS of on and the same of off, so the LED has to be
 * looked at several times inside each half or the phase lands wherever it
 * happens to. A quarter of the shorter of the two rhythms.
 */
#define TK_STATUS_TICK_MS MAX(CONFIG_TK_STATUS_LED_BLINK_MS / 4, 10)

static void status_tick(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(tick_work, status_tick);

static void apply(uint8_t colour)
{
    const bool red = colour == TK_STATUS_RED || colour == TK_STATUS_AMBER;
    const bool green = colour == TK_STATUS_GREEN || colour == TK_STATUS_AMBER;

    (void) gpio_pin_set_dt(&led_red, red);
    (void) gpio_pin_set_dt(&led_green, green);
}

static void status_tick(struct k_work *work)
{
    ARG_UNUSED(work);

    const int64_t now = k_uptime_get();

    apply(tk_status_output(now));

    if (tk_status_animating(now)) {
        (void) k_work_reschedule(&tick_work, K_MSEC(TK_STATUS_TICK_MS));
    }
}

/** Something changed. Look now, and keep looking if it moves. */
static void refresh(void)
{
    (void) k_work_reschedule(&tick_work, K_NO_WAIT);
}

/*
 * Runs on the workqueue, since chan_power is published from there. Setting the
 * pins from here directly would be correct today and wrong the moment anything
 * publishes from another context, so it goes through the same work item as
 * everything else.
 */
static void on_power(const struct zbus_channel *chan)
{
    const struct tk_power_msg *msg = zbus_chan_const_msg(chan);

    /*
     * The portal is asked here rather than pushed from `net`: it has no event
     * to push on — it goes on air inside a state machine that has no reason to
     * know about LEDs — and every power sample comes past this point anyway.
     */
    tk_status_post_portal(tk_net_is_active());
    tk_status_post_power(msg->state, false);

    refresh();
}

ZBUS_LISTENER_DEFINE(tk_status_obs, on_power);
ZBUS_CHAN_ADD_OBS(chan_power, tk_status_obs, 5);

void tk_status_set_activity(bool busy)
{
    tk_status_post_activity(busy);
    refresh();
}

void tk_status_note_refresh_blocked(void)
{
    /*
     * The state is not carried in: the arbiter already has it from the last
     * sample, and passing it again from the app thread would be a second
     * source of truth for the same fact.
     */
    tk_status_post_power(tk_power_state(), true);
    refresh();
}

void tk_status_off(void)
{
    (void) k_work_cancel_delayable(&tick_work);

    apply(TK_STATUS_OFF);
}

static int status_start(void)
{
    if (!gpio_is_ready_dt(&led_red) || !gpio_is_ready_dt(&led_green)) {
        LOG_ERR("status LEDs are not ready; running without them");
        return 0;
    }

    (void) gpio_pin_configure_dt(&led_red, GPIO_OUTPUT_INACTIVE);
    (void) gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_INACTIVE);

    /*
     * Not lit at boot, and not blinked either. A wake is a fresh boot, so a
     * hello here would fire on every press — the LEDs say something when there
     * is something to say and are dark the rest of the time.
     */
    refresh();

    return 0;
}

SYS_INIT(status_start, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
