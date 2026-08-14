/* The synced corpus on the filesystem. */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "corpus.h"

LOG_MODULE_REGISTER(tk_corpus, LOG_LEVEL_INF);

/* Buffer the largest corpus accepted by QDB and panel limits. */
static uint8_t corpus_buf[CONFIG_TK_MAX_CORPUS_BYTES];
static size_t corpus_len;

/* `/corpus/en.qdb`, and the staging name beside it. */
static void corpus_path(const char *code, const char *suffix, char *out, size_t out_size)
{
    (void) snprintf(out, out_size, TK_CORPUS_DIR "/%s.qdb%s", code, suffix);
}

uint8_t *tk_corpus_buffer(size_t *capacity)
{
    if (capacity != NULL) {
        *capacity = sizeof(corpus_buf);
    }

    return corpus_buf;
}

const uint8_t *tk_corpus_stored(const char *code, size_t *size)
{
    char path[48];
    struct fs_dirent info;
    struct fs_file_t file;

    if (code == NULL || size == NULL) {
        return NULL;
    }

    corpus_path(code, "", path, sizeof(path));

    if (fs_stat(path, &info) != 0) {
        /* No synced corpus for this language. */
        return NULL;
    }

    if (info.size == 0 || (size_t) info.size > sizeof(corpus_buf)) {
        LOG_ERR("%s is %zu bytes; the buffer holds %zu", path, (size_t) info.size,
                sizeof(corpus_buf));
        return NULL;
    }

    fs_file_t_init(&file);

    int err = fs_open(&file, path, FS_O_READ);

    if (err != 0) {
        LOG_ERR("could not open %s: %d", path, err);
        return NULL;
    }

    const ssize_t n = fs_read(&file, corpus_buf, (size_t) info.size);

    (void) fs_close(&file);

    if (n < 0 || (size_t) n != (size_t) info.size) {
        LOG_ERR("%s read back short: %zd of %zu", path, n, (size_t) info.size);
        return NULL;
    }

    corpus_len = (size_t) n;
    *size = corpus_len;

    LOG_INF("corpus %s read from the filesystem: %zu bytes", code, corpus_len);

    return corpus_buf;
}

int tk_corpus_store(const char *code, const uint8_t *data, size_t size)
{
    char path[48];
    char staging[48];
    struct fs_file_t file;

    if (code == NULL || data == NULL || size == 0) {
        return -EINVAL;
    }

    if (size > sizeof(corpus_buf)) {
        /* Validate a corpus before it can replace the stored copy. */
        LOG_ERR("a %zu-byte corpus will not fit the %zu-byte buffer", size, sizeof(corpus_buf));
        return -EFBIG;
    }

    corpus_path(code, "", path, sizeof(path));
    corpus_path(code, ".new", staging, sizeof(staging));

    fs_file_t_init(&file);

    int err = fs_open(&file, staging, FS_O_CREATE | FS_O_WRITE | FS_O_TRUNC);

    if (err != 0) {
        LOG_ERR("could not open %s: %d", staging, err);
        return err;
    }

    const ssize_t n = fs_write(&file, data, size);

    err = fs_close(&file);

    if (n < 0 || (size_t) n != size) {
        LOG_ERR("%s wrote short: %zd of %zu", staging, n, size);
        (void) fs_unlink(staging);

        return n < 0 ? (int) n : -EIO;
    }

    if (err != 0) {
        LOG_ERR("could not close %s: %d", staging, err);
        (void) fs_unlink(staging);

        return err;
    }

    /* The swap. */
    err = fs_rename(staging, path);

    if (err != 0) {
        LOG_ERR("could not rename %s over %s: %d", staging, path, err);
        (void) fs_unlink(staging);

        return err;
    }

    LOG_INF("corpus %s written to the filesystem: %zu bytes", code, size);

    return 0;
}
