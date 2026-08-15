/* The `kveld` settings subtree: the chosen language, and the installed corpus release. */

#include "language.h"

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "corpus.h"

LOG_MODULE_REGISTER(kveld_language, LOG_LEVEL_INF);

static char chosen[KVELD_LANGUAGE_LEN] = CONFIG_KVELD_CORPUS_LANGUAGE;
static char installed[KVELD_CORPUS_VERSION_LEN];

bool kveld_language_available(const char *code)
{
    if (code == NULL) {
        return false;
    }

    for (size_t i = 0; i < kveld_corpus_count(); i++) {
        if (strcmp(kveld_corpus_language(i), code) == 0) {
            return true;
        }
    }

    return false;
}

const char *kveld_language(void)
{
    return chosen;
}

const char *kveld_corpus_version(void)
{
    return installed;
}

#ifdef CONFIG_SETTINGS

#include <zephyr/settings/settings.h>

#define LANGUAGE_KEY "kveld/lang"
#define VERSION_KEY "kveld/corpus_ver"

/* Called by the settings subsystem for each key under `kveld`. */
static int language_load(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
    if (strcmp(name, "corpus_ver") == 0) {
        if (len >= sizeof(installed)) {
            LOG_WRN("stored corpus version is %u bytes; ignoring it", (unsigned int) len);
            return -EINVAL;
        }

        const ssize_t n = read_cb(cb_arg, installed, len);

        if (n < 0) {
            return (int) n;
        }

        installed[n] = '\0';
        LOG_INF("installed corpus: %s", installed);

        return 0;
    }

    if (strcmp(name, "lang") != 0) {
        return -ENOENT;
    }

    char stored[KVELD_LANGUAGE_LEN] = {0};

    if (len >= sizeof(stored)) {
        LOG_WRN("stored language is %u bytes; ignoring it", (unsigned int) len);
        return -EINVAL;
    }

    const ssize_t n = read_cb(cb_arg, stored, len);

    if (n < 0) {
        return (int) n;
    }

    stored[n] = '\0';

    /* Ignore stored languages without a compiled corpus. */
    if (!kveld_language_available(stored)) {
        LOG_WRN("stored language %s is not in this image; keeping %s", stored, chosen);
        return 0;
    }

    strcpy(chosen, stored);
    LOG_INF("language: %s", chosen);

    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(kveld_language, "kveld", NULL, language_load, NULL, NULL);

int kveld_language_set(const char *code)
{
    if (!kveld_language_available(code)) {
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

int kveld_corpus_version_set(const char *version)
{
    if (version == NULL || strlen(version) >= sizeof(installed)) {
        return -EINVAL;
    }

    const int err = settings_save_one(VERSION_KEY, version, strlen(version));

    if (err != 0) {
        LOG_ERR("could not record the installed corpus version: %d", err);
        return err;
    }

    strcpy(installed, version);

    return 0;
}

#else

int kveld_corpus_version_set(const char *version)
{
    ARG_UNUSED(version);

    return -ENOTSUP;
}

int kveld_language_set(const char *code)
{
    ARG_UNUSED(code);

    /* Report persistence as unsupported without settings. */
    return -ENOTSUP;
}

#endif /* CONFIG_SETTINGS */
