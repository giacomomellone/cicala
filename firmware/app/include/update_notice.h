/*
 * The card that says the firmware changed. See src/update_notice.c.
 */

#ifndef TK_UPDATE_NOTICE_H
#define TK_UPDATE_NOTICE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Compare the running firmware against the last version seen on this device,
 * and put a card on the panel when they differ.
 *
 * Call once per boot, after the settings subsystem has loaded and with the
 * zbus channels up. Silent on a device that has never recorded a version, and
 * silent on every boot after the first of a given version.
 */
void tk_update_notice_check(void);

#ifdef __cplusplus
}
#endif

#endif /* TK_UPDATE_NOTICE_H */
