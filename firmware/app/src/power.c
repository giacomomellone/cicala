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

/** True when VBUS is high. A read error is reported as unplugged. */
static bool read_vbus(void)
{
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

    uint16_t mv = 0;

    if (read_pack_mv(&mv)) {
        tk_power_post_sample(mv, read_vbus());
    } else {
        /*
         * Still tick the machine. It owns the charge window, and a window that
         * stopped closing because the ADC failed would hold the device awake
         * on a charger indefinitely.
         */
        tk_power_run();
    }

    (void) k_work_reschedule(&sample_work, K_MSEC(TK_POWER_INTERVAL_MS));
}

static int power_start(void)
{
    if (!adc_is_ready_dt(&battery)) {
        LOG_ERR("ADC %s is not ready; running without a battery reading",
                battery.dev != NULL ? battery.dev->name : "?");
        return 0;
    }

    const int err = adc_channel_setup_dt(&battery);

    if (err != 0) {
        LOG_ERR("could not set up ADC channel %u: %d", battery.channel_id, err);
        return 0;
    }

    if (!gpio_is_ready_dt(&vbus)) {
        LOG_ERR("VBUS pin is not ready; the device will believe it is on battery");
    } else {
        (void) gpio_pin_configure_dt(&vbus, GPIO_INPUT);
    }

    /*
     * Synchronous, and before any thread starts. See the header comment: `net`
     * reads VBUS as it starts up and would otherwise be told UNKNOWN.
     */
    power_sample(NULL);

    return 0;
}

/*
 * The same hook sleep_start() uses, and for the same reason: it is the last
 * init level, so every driver this touches is up, and it still runs before the
 * static threads.
 */
SYS_INIT(power_start, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
