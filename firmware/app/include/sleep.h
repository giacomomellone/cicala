/** What woke the device. */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

enum tk_wake_source {
    /** Not a wake: a power-on, a reset, or an image that never sleeps. */
    TK_WAKE_NONE = 0,
    TK_WAKE_CATEGORY,
    TK_WAKE_NEXT,
};

#ifdef CONFIG_TK_SLEEP

/** Which button ended the last sleep. */
enum tk_wake_source tk_wake_button(void);

#else

/** The awake image never sleeps, so every boot is a cold one. */
static inline enum tk_wake_source tk_wake_button(void)
{
    return TK_WAKE_NONE;
}

#endif /* CONFIG_TK_SLEEP */

#ifdef __cplusplus
}
#endif
