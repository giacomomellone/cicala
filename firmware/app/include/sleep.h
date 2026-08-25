/** What woke the device. */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

enum cicala_wake_source {
    /** Not a wake: a power-on, a reset, or an image that never sleeps. */
    CICALA_WAKE_NONE = 0,
    CICALA_WAKE_CATEGORY,
    CICALA_WAKE_NEXT,
};

#ifdef CONFIG_CICALA_SLEEP

/** Which button ended the last sleep. */
enum cicala_wake_source cicala_wake_button(void);

#else

/** The awake image never sleeps, so every boot is a cold one. */
static inline enum cicala_wake_source cicala_wake_button(void)
{
    return CICALA_WAKE_NONE;
}

#endif /* CONFIG_CICALA_SLEEP */

#ifdef __cplusplus
}
#endif
