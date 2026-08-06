/*
 * The synced corpus on the filesystem.
 *
 * Compiled only when there is a filesystem to read, which on this board is the
 * LittleFS partition above the 4 MB line — see the board overlay. Without one
 * the compiled-in corpora are all there is, and corpus.h says so inline.
 *
 * ## Why the whole file is read into RAM
 *
 * `qdb` is a zero-copy reader: `Question::text` points into the bundle and
 * stays valid for as long as the store is open, which is what keeps a draw
 * free of allocation. A file's bytes are not contiguous in flash, so there is
 * nothing to point at until they are somewhere that is.
 *
 * One buffer, reused. Only one language is ever open, so a second read
 * replaces the first rather than needing room for both.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "corpus.h"

LOG_MODULE_REGISTER(tk_corpus, LOG_LEVEL_INF);

/*
 * Sized against CONFIG_TK_MAX_QUESTIONS rather than against what ships today:
 * the largest corpus qdb will accept is 512 questions, and the longest
 * question it will render is CONFIG_TK_MAX_QUESTION_BYTES. English is 15 KB at
 * 240 questions, so this has room for a corpus twice that size.
 */
static uint8_t corpus_buf[CONFIG_TK_MAX_CORPUS_BYTES];
static size_t corpus_len;

/** `/corpus/en.qdb`, and the staging name beside it. */
static void corpus_path(const char *code, const char *suffix, char *out, size_t out_size)
{
    (void) snprintf(out, out_size, TK_CORPUS_DIR "/%s.qdb%s", code, suffix);
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
        /* No synced corpus for this language. The ordinary case on a device
         * that has never reached a network, and not worth a log line. */
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
        /* Refused here rather than after it is on disk: a corpus this device
         * cannot read back is worse stored than not stored, because it would
         * replace one that works. */
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

    /*
     * The swap. fs_rename replaces the destination, so there is no window in
     * which neither file exists — a power cut either side of this leaves a
     * whole corpus on disk, the old one or the new one.
     */
    err = fs_rename(staging, path);

    if (err != 0) {
        LOG_ERR("could not rename %s over %s: %d", staging, path, err);
        (void) fs_unlink(staging);

        return err;
    }

    LOG_INF("corpus %s written to the filesystem: %zu bytes", code, size);

    return 0;
}
