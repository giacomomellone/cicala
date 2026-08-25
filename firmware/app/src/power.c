/* Battery ADC and VBUS sampling. */

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "app_logic.h"
#include "power.h"
#include "power_logic.h"

LOG_MODULE_REGISTER(cicala_power_adc, LOG_LEVEL_INF);

#define CICALA_USER_NODE DT_PATH(zephyr_user)

static const struct adc_dt_spec battery = ADC_DT_SPEC_GET(CICALA_USER_NODE);

static const struct gpio_dt_spec vbus = GPIO_DT_SPEC_GET(CICALA_USER_NODE, cicala_vbus_gpios);

/* Suspend samples while panel load causes pack-voltage sag. */
#define CICALA_POWER_BUSY_RETRY_MS 250

#if IS_ENABLED(CONFIG_CICALA_DEBUG_POWER)
#define CICALA_POWER_INTERVAL_MS 1000
#else
#define CICALA_POWER_INTERVAL_MS CONFIG_CICALA_POWER_SAMPLE_MS
#endif

static void power_sample(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(sample_work, power_sample);

/* What came up at boot, checked before each is used. */
static bool adc_ready;
static bool vbus_ready;

/* One conversion, in millivolts at the pin. */
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

    if (IS_ENABLED(CONFIG_CICALA_DEBUG_POWER)) {
        LOG_INF("raw %u -> %d mV at the pin", raw, value);
    }

    *mv = value;

    return true;
}

/* The cell voltage, averaged over a burst and scaled back through the divider. */
static bool read_pack_mv(uint16_t *mv)
{
    if (!adc_ready) {
        /* power_start() reports persistent ADC configuration failures once. */
        return false;
    }

    int32_t total = 0;
    int taken = 0;

    for (int i = 0; i < CONFIG_CICALA_POWER_CONFIRM_SAMPLES; i++) {
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
    const int32_t pack = tap * CONFIG_CICALA_POWER_DIVIDER_NUM / CONFIG_CICALA_POWER_DIVIDER_DEN;

    *mv = (uint16_t) pack;

    return true;
}

/* True when VBUS is high. */
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

    if (cicala_app_is_busy()) {
        (void) k_work_reschedule(&sample_work, K_MSEC(CICALA_POWER_BUSY_RETRY_MS));
        return;
    }

    const bool usb = read_vbus();

    uint16_t mv = 0;

    if (read_pack_mv(&mv)) {
        cicala_power_post_sample(mv, usb);
    } else {
        /* Preserve VBUS transitions when the ADC is unavailable. */
        cicala_power_post_usb(usb);
    }

    (void) k_work_reschedule(&sample_work, K_MSEC(CICALA_POWER_INTERVAL_MS));
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

    /* Synchronous, and before any thread starts. */
    power_sample(NULL);

    if (cicala_power_external() && cicala_power_millivolts() < CONFIG_CICALA_POWER_PLAUSIBLE_MV) {
        /* This combination usually means both divider inputs are floating. */
        LOG_WRN("VBUS is high but the pack reads %u mV; check both dividers are fitted",
                cicala_power_millivolts());
    }

    return 0;
}

/* Run after device initialization and before static threads. */
SYS_INIT(power_start, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
