/** The card that says the firmware changed. */

#ifndef TK_UPDATE_NOTICE_H
#define TK_UPDATE_NOTICE_H

#ifdef __cplusplus
extern "C" {
#endif

/** Show a card when the running version differs from the recorded version. */
void tk_update_notice_check(void);

#ifdef __cplusplus
}
#endif

#endif /* TK_UPDATE_NOTICE_H */
