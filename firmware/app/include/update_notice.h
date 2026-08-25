/** The card that says the firmware changed. */

#ifndef CICALA_UPDATE_NOTICE_H
#define CICALA_UPDATE_NOTICE_H

#ifdef __cplusplus
extern "C" {
#endif

/** Show a card when the running version differs from the recorded version. */
void cicala_update_notice_check(void);

#ifdef __cplusplus
}
#endif

#endif /* CICALA_UPDATE_NOTICE_H */
