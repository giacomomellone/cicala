/** What the rest of the firmware needs to know about `net`. */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_CICALA_NET

/** True while anything is on air. */
bool cicala_net_is_active(void);

/** The form was posted and the credentials are stored. */
void cicala_net_notify_credentials(void);

/** Somebody pressed "sync now" in the portal. */
void cicala_net_notify_sync(void);

#else

/** No radio in this image, so nothing is ever on air. */
static inline bool cicala_net_is_active(void)
{
    return false;
}

#endif /* CONFIG_CICALA_NET */

#ifdef __cplusplus
}
#endif
