/*
 * What woke the device.
 *
 * Deep sleep is a reboot, so the press that ends a sleep is spent on the wake
 * itself: EXT1 sees the edge before the kernel exists, and by the time the
 * gpio-keys driver is listening the button is already down. src/input.c
 * deliberately refuses to invent a press out of the release that follows, so
 * without this the wake press reaches nobody and the device answers the first
 * press of every conversation by doing nothing at all.
 *
 * The wake mask is the record of that press, and it is the only one there is.
 */

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

/**
 * Which button ended the last sleep.
 *
 * Latched before the kernel starts and constant thereafter, so it can be read
 * whenever it is convenient rather than at a particular moment in boot.
 *
 * Both buttons at once reports TK_WAKE_NEXT: Next is the one with a visible
 * answer, and a device that draws a question when someone grabs it with two
 * fingers is better than one that does nothing.
 */
enum tk_wake_source tk_wake_button(void);

#else

/* The awake image never sleeps, so every boot is a cold one. Inline rather
 * than a second translation unit: it lets app_logic.cpp ask the question
 * unconditionally and lets the compiler drop the branch. */
static inline enum tk_wake_source tk_wake_button(void)
{
    return TK_WAKE_NONE;
}

#endif /* CONFIG_TK_SLEEP */

#ifdef __cplusplus
}
#endif
