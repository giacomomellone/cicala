/*
 * The chosen language, and where it is kept.
 *
 * C rather than C++ because SETTINGS_STATIC_HANDLER_DEFINE expands to a
 * designated initializer the same way the zbus macros do.
 */

#include "language.h"

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "corpus.h"

LOG_MODULE_REGISTER(tk_language, LOG_LEVEL_INF);

static char chosen[TK_LANGUAGE_LEN] = CONFIG_TK_CORPUS_LANGUAGE;

bool tk_language_available(const char *code)
{
    if (code == NULL) {
        return false;
    }

    for (size_t i = 0; i < tk_corpus_count(); i++) {
        if (strcmp(tk_corpus_language(i), code) == 0) {
            return true;
        }
    }

    return false;
}

const char *tk_language(void)
{
    return chosen;
}

#ifdef CONFIG_SETTINGS

#include <zephyr/settings/settings.h>

#define LANGUAGE_KEY "tk/lang"

/** Called by the settings subsystem for each key under `tk`. */
static int language_load(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
    if (strcmp(name, "lang") != 0) {
        return -ENOENT;
    }

    char stored[TK_LANGUAGE_LEN] = {0};

    if (len >= sizeof(stored)) {
        LOG_WRN("stored language is %u bytes; ignoring it", (unsigned int) len);
        return -EINVAL;
    }

    const ssize_t n = read_cb(cb_arg, stored, len);

    if (n < 0) {
        return (int) n;
    }

    stored[n] = '\0';

    /*
     * A language the image no longer carries is dropped rather than obeyed.
     * Firmware can ship with a different set than the one that stored this,
     * and a device with no corpus to open would have nothing to draw at all.
     */
    if (!tk_language_available(stored)) {
        LOG_WRN("stored language %s is not in this image; keeping %s", stored, chosen);
        return 0;
    }

    strcpy(chosen, stored);
    LOG_INF("language: %s", chosen);

    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(tk_language, "tk", NULL, language_load, NULL, NULL);

int tk_language_set(const char *code)
{
    if (!tk_language_available(code)) {
        LOG_WRN("refusing a language this image does not carry: %s",
                code != NULL ? code : "(null)");
        return -EINVAL;
    }

    if (strcmp(chosen, code) == 0) {
        return 0;
    }

    const int err = settings_save_one(LANGUAGE_KEY, code, strlen(code));

    if (err != 0) {
        LOG_ERR("could not store the language: %d", err);
        return err;
    }

    strcpy(chosen, code);
    LOG_INF("language is now %s", chosen);

    return 0;
}

#else

int tk_language_set(const char *code)
{
    ARG_UNUSED(code);

    /* Nowhere to keep it, so nothing may claim it was kept. */
    return -ENOTSUP;
}

#endif /* CONFIG_SETTINGS */
