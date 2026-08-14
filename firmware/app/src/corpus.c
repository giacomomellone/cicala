/* The embedded corpora, as a table. */

#include "corpus.h"

#include <string.h>

#include <zephyr/sys/util.h>

static const uint8_t corpus_en[] = {
#include "en_qdb.inc"
};

static const uint8_t corpus_de[] = {
#include "de_qdb.inc"
};

static const struct {
    const char *language;
    const uint8_t *data;
    size_t size;
} corpora[] = {
    {"en", corpus_en, sizeof(corpus_en)},
    {"de", corpus_de, sizeof(corpus_de)},
};

size_t tk_corpus_count(void)
{
    return ARRAY_SIZE(corpora);
}

const char *tk_corpus_language(size_t index)
{
    if (index >= ARRAY_SIZE(corpora)) {
        return NULL;
    }

    return corpora[index].language;
}

const uint8_t *tk_corpus_data(size_t index, size_t *size)
{
    if (index >= ARRAY_SIZE(corpora)) {
        return NULL;
    }

    *size = corpora[index].size;

    return corpora[index].data;
}

int tk_corpus_find(const char *code)
{
    if (code == NULL) {
        return -1;
    }

    for (size_t i = 0; i < ARRAY_SIZE(corpora); i++) {
        if (strcmp(corpora[i].language, code) == 0) {
            return (int) i;
        }
    }

    return -1;
}
