/** What woke the device. */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

enum kveld_wake_source {
    /** Not a wake: a power-on, a reset, or an image that never sleeps. */
    KVELD_WAKE_NONE = 0,
    KVELD_WAKE_CATEGORY,
    KVELD_WAKE_NEXT,
};

#ifdef CONFIG_KVELD_SLEEP

/** Which button ended the last sleep. */
enum kveld_wake_source kveld_wake_button(void);

#else

/** The awake image never sleeps, so every boot is a cold one. */
static inline enum kveld_wake_source kveld_wake_button(void)
{
    return KVELD_WAKE_NONE;
}

#endif /* CONFIG_KVELD_SLEEP */

#ifdef __cplusplus
}
#endif
