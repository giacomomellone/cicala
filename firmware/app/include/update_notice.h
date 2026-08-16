/** The card that says the firmware changed. */

#ifndef KVELD_UPDATE_NOTICE_H
#define KVELD_UPDATE_NOTICE_H

#ifdef __cplusplus
extern "C" {
#endif

/** Show a card when the running version differs from the recorded version. */
void kveld_update_notice_check(void);

#ifdef __cplusplus
}
#endif

#endif /* KVELD_UPDATE_NOTICE_H */
