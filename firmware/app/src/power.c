/*
 * The ADC, the VBUS pin, and when to look at them.
 *
 * Compiled only when CONFIG_TK_POWER is on, which the devkit board conf sets.
 * What a reading *means* is lib/power's job, reached through power_logic.h;
 * this file only produces honest numbers and decides how often.
 *
 * ## Why the system workqueue, and not a thread or the app loop
 *
 * One conversion plus one pin read is microseconds and never blocks, so it does
 * not earn a thread and a stack. It cannot live on the `app` thread either:
 * that thread parks in K_FOREVER when there is nothing to do, and that is
 * exactly the property which lets the device reach deep sleep. Giving it a
 * periodic timeout so power could be sampled would trade the sleep budget for a
 * reading nobody is waiting for.
 *
 * ## Why the boot reading is synchronous
 *
 * `net` decides whether to join a network from whether VBUS is high, and it
 * decides it in the first moments of the thread starting. Static threads run
 * after every SYS_INIT level, so a reading taken here — in the same hook
 * sleep.c uses — is on the channel before anything can ask. Scheduling the work
 * item instead would race, and lose about half the time.
 */

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "app_logic.h"
#include "power.h"
#include "power_logic.h"

LOG_MODULE_REGISTER(tk_power_adc, LOG_LEVEL_INF);

#define TK_USER_NODE DT_PATH(zephyr_user)

static const struct adc_dt_spec battery = ADC_DT_SPEC_GET(TK_USER_NODE);

static const struct gpio_dt_spec vbus = GPIO_DT_SPEC_GET(TK_USER_NODE, tk_vbus_gpios);

/*
 * A refresh sags the pack by hundreds of millivolts for two seconds, so a
 * reading taken during one is not a reading of the cell. Rescheduling short
 * rather than skipping the slot: the interesting moment is usually just after a
 * refresh, when the cell is recovering.
 */
#define TK_POWER_BUSY_RETRY_MS 250

#if IS_ENABLED(CONFIG_TK_DEBUG_POWER)
#define TK_POWER_INTERVAL_MS 1000
#else
#define TK_POWER_INTERVAL_MS CONFIG_TK_POWER_SAMPLE_MS
#endif

static void power_sample(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(sample_work, power_sample);

/*
 * What came up at boot, checked before each is used.
 *
 * The two are independent on purpose. VBUS is a plain GPIO on its own pin, so
 * an ADC that never came ready must not take the USB bit down with it — that
 * bit is what `net` joins a network on and what keeps the device from sleeping
 * on a charger, and freezing it is worse than not having it.
 */
static bool adc_ready;
static bool vbus_ready;

/**
 * One conversion, in millivolts at the pin.
 *
 * @return false when the ADC would not answer, which leaves the machine in
 *         UNKNOWN — a state that permits refreshes, so a broken divider costs
 *         the battery reading and nothing else.
 */
static bool read_tap_mv(int32_t *mv)
{
    uint16_t raw = 0;

    struct adc_sequence sequence = {
        .buffer = &raw,
        .buffer_size = sizeof(raw),
    };

    int err = adc_sequence_init_dt(&battery, &sequence);

    if (err != 0) {
        LOG_ERR("could not set up the conversion: %d", err);
        return false;
    }

    err = adc_read_dt(&battery, &sequence);

    if (err != 0) {
        LOG_ERR("could not read the battery tap: %d", err);
        return false;
    }

    int32_t value = (int32_t) raw;

    err = adc_raw_to_millivolts_dt(&battery, &value);

    if (err != 0) {
        LOG_ERR("could not scale the reading: %d", err);
        return false;
    }

    if (IS_ENABLED(CONFIG_TK_DEBUG_POWER)) {
        LOG_INF("raw %u -> %d mV at the pin", raw, value);
    }

    *mv = value;

    return true;
}

/**
 * The cell voltage, averaged over a burst and scaled back through the divider.
 *
 * All of the smoothing lives here rather than in the state machine, because
 * this is the only place that gets more than one reading. On battery the device
 * is awake for about two seconds per press, so the machine sees one sample in
 * its whole life; averaging by sample count inside it would never run.
 */
static bool read_pack_mv(uint16_t *mv)
{
    if (!adc_ready) {
        /* Silently: power_start() has already said why, and repeating it every
         * CONFIG_TK_POWER_SAMPLE_MS would bury whatever else the console has
         * to say. */
        return false;
    }

    int32_t total = 0;
    int taken = 0;

    for (int i = 0; i < CONFIG_TK_POWER_CONFIRM_SAMPLES; i++) {
        int32_t one = 0;

        if (!read_tap_mv(&one)) {
            continue;
        }

        total += one;
        taken++;
    }

    if (taken == 0) {
        return false;
    }

    const int32_t tap = total / taken;
    const int32_t pack = tap * CONFIG_TK_POWER_DIVIDER_NUM / CONFIG_TK_POWER_DIVIDER_DEN;

    *mv = (uint16_t) pack;

    return true;
}

/** True when VBUS is high. A pin that never came ready reads as unplugged. */
static bool read_vbus(void)
{
    if (!vbus_ready) {
        return false;
    }

    const int level = gpio_pin_get_dt(&vbus);

    if (level < 0) {
        LOG_WRN("could not read VBUS: %d", level);
        return false;
    }

    return level > 0;
}

static void power_sample(struct k_work *work)
{
    ARG_UNUSED(work);

    if (tk_app_is_busy()) {
        (void) k_work_reschedule(&sample_work, K_MSEC(TK_POWER_BUSY_RETRY_MS));
        return;
    }

    /*
     * Read on every tick, in both branches. VBUS is its own pin and costs one
     * register read, so the state of the ADC has no business deciding whether
     * anybody hears about it: a bit that froze at "plugged in" would keep
     * tk_power_external() true, and sleep.c refuses to sleep while that holds.
     */
    const bool usb = read_vbus();

    uint16_t mv = 0;

    if (read_pack_mv(&mv)) {
        tk_power_post_sample(mv, usb);
    } else {
        /*
         * Still tick the machine. It owns the charge window, and a window that
         * stopped closing because the ADC failed would hold the device awake
         * on a charger indefinitely.
         */
        tk_power_post_usb(usb);
    }

    (void) k_work_reschedule(&sample_work, K_MSEC(TK_POWER_INTERVAL_MS));
}

static int power_start(void)
{
    if (!adc_is_ready_dt(&battery)) {
        LOG_ERR("ADC %s is not ready; running without a battery reading",
                battery.dev != NULL ? battery.dev->name : "?");
    } else if (adc_channel_setup_dt(&battery) != 0) {
        LOG_ERR("could not set up ADC channel %u; running without a battery reading",
                battery.channel_id);
    } else {
        adc_ready = true;
    }

    if (!gpio_is_ready_dt(&vbus)) {
        LOG_ERR("VBUS pin is not ready; the device will believe it is on battery");
    } else {
        (void) gpio_pin_configure_dt(&vbus, GPIO_INPUT);
        vbus_ready = true;
    }

    /*
     * Synchronous, and before any thread starts. Reached whether or not the
     * ADC came up: see the header comment — `net` reads VBUS as it starts and
     * `status` listens for the first publish on chan_power, and neither of
     * those depends on there being a conversion. This call is also what
     * schedules the work item, so everything after it hangs off reaching here.
     */
    power_sample(NULL);

    if (tk_power_external() && tk_power_millivolts() < CONFIG_TK_POWER_PLAUSIBLE_MV) {
        /*
         * VBUS reads high and the pack reads like nothing at all, which on a
         * bench rig usually means neither divider is fitted and both pins are
         * floating. Worth saying, because the consequence is quiet: external
         * power inhibits sleep, so the device simply never sleeps again.
         */
        LOG_WRN("VBUS is high but the pack reads %u mV; check both dividers are fitted",
                tk_power_millivolts());
    }

    return 0;
}

/*
 * The same hook sleep_start() uses, and for the same reason: it is the last
 * init level, so every driver this touches is up, and it still runs before the
 * static threads.
 */
SYS_INIT(power_start, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
