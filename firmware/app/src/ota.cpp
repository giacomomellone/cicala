/*
 * Fetching a firmware image, and refusing every image that is not exactly
 * right.
 *
 * The order of the checks is the same one src/sync.cpp uses and is worth
 * reading twice: size, then SHA-256, then signature. Cheapest first. What
 * differs is that a bundle is checked before it touches storage and an image
 * cannot be — 780 KB does not fit in RAM, so it is written to the spare slot as
 * it arrives and judged afterwards. That is safe because the spare slot is
 * scratch: nothing boots from it, and an image that fails any check is simply
 * never marked for install. The running firmware is untouched either way.
 *
 * Two signatures cover an update, and they say different things:
 *
 *   - the one here, made with the bundle key, says "the project published this
 *     image and it is the current release". It is what lets the device refuse a
 *     bad image before spending a reboot on it.
 *   - MCUboot's, made with the separate firmware key, says "this image may
 *     run". It is checked by the bootloader on the next boot, and it is the one
 *     that actually gates execution.
 *
 * C++ rather than C for the same reason src/sync.cpp is: the manifest parser
 * and the verifier are lib/ code in namespace tk.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/dfu/flash_img.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>

#include <app_version.h>
#include <psa/crypto.h>

#include "ed25519.hpp"
#include "fetch.h"
#include "manifest.hpp"
#include "ota.h"

extern "C" {
#include "trusted_key.h"
}

LOG_MODULE_REGISTER(tk_ota, LOG_LEVEL_INF);

/* A firmware manifest is one flat object of six fields. Room for several times
 * that, and a bound on what a hostile server can make us hold. */
#define MANIFEST_MAX 1024

static char manifest_buf[MANIFEST_MAX];

/*
 * What the image is written through, and what the running hash is kept in.
 *
 * File-scope and static because both are large — flash_img_context carries a
 * CONFIG_IMG_BLOCK_BUF_SIZE staging buffer — and because the sink callback
 * needs to reach them without a context struct of its own. There is one OTA in
 * flight at a time; both callers run on the `net` thread.
 */
static struct flash_img_context img_ctx;
static psa_hash_operation_t hash_op;
static bool sink_failed;

const char *tk_ota_running_version(void)
{
    return APP_VERSION_STRING;
}

/**
 * One chunk: into the slot, and into the digest.
 *
 * Hashing here rather than reading the slot back afterwards. Re-reading would
 * be the more paranoid choice — it would catch a flash write that lied — but
 * MCUboot re-hashes the whole image against its own header before it installs
 * anything, so a bad write is caught by the layer whose job that is.
 */
static int on_chunk(void *ctx, const uint8_t *data, size_t len)
{
    ARG_UNUSED(ctx);

    if (psa_hash_update(&hash_op, data, len) != PSA_SUCCESS) {
        LOG_ERR("could not hash the image");
        sink_failed = true;

        return -EIO;
    }

    /* `false` is "not the last chunk": flash_img only needs to know so it can
     * pad the final write out to a write-block boundary, and tk_fetch has no
     * way of knowing which chunk is last until the response ends. The flush
     * below is what closes the image out. */
    const int err = flash_img_buffered_write(&img_ctx, data, len, false);

    if (err != 0) {
        LOG_ERR("could not write to the spare slot: %d", err);
        sink_failed = true;

        return err;
    }

    return 0;
}

enum tk_ota_result tk_ota_run(char *version, size_t version_size)
{
    tk::FirmwareRelease release = {};

    LOG_INF("checking %s for firmware newer than %s", CONFIG_TK_OTA_BASE_URL, APP_VERSION_STRING);

    struct tk_fetch_mem mem = {};

    mem.buf = (uint8_t *) manifest_buf;
    mem.capacity = sizeof(manifest_buf);

    const struct tk_fetch_sink manifest_sink = {tk_fetch_mem_write, &mem};

    int n = tk_fetch(tk_fetch_path_of(CONFIG_TK_OTA_BASE_URL "/firmware.json"), &manifest_sink,
                     TK_FETCH_TIMEOUT_MS);

    if (n < 0) {
        return TK_OTA_FAILED;
    }

    if (!tk::firmware_parse(manifest_buf, (size_t) n, release)) {
        LOG_ERR("the firmware manifest did not parse");

        return TK_OTA_FAILED;
    }

    if (release.schema != tk::kFirmwareSchema) {
        /* A schema this firmware does not know could mean anything, including
         * that a field it relies on now means something else. */
        LOG_ERR("firmware manifest schema %u, expected %u", release.schema, tk::kFirmwareSchema);

        return TK_OTA_FAILED;
    }

    /*
     * The anti-rollback rule, and the reason an unauthenticated transport is
     * survivable: an attacker can replay an older manifest, signature and all,
     * but cannot make it look newer. Equal counts as current, so a device that
     * has already installed a release does not fetch it again on every boot.
     */
    if (tk::version_compare(release.version, APP_VERSION_STRING) <= 0) {
        LOG_INF("running %s and the manifest offers %s — nothing to do", APP_VERSION_STRING,
                release.version);

        return TK_OTA_CURRENT;
    }

    if (!release.signed_) {
        /* An image built without the manifest key. A device must never install
         * one; see docs/firmware_update.md. */
        LOG_ERR("refusing an unsigned firmware release");

        return TK_OTA_FAILED;
    }

    /*
     * Refused before the download rather than after: the slot's size is known
     * here, and an image that cannot fit is not worth several minutes of radio.
     */
    /*
     * Not the slot's size: the usable size. MCUboot writes the trailer that
     * marks an image pending at the end of the slot, so an image filling the
     * slot exactly would have nowhere to be marked and would be written over
     * its own status bytes.
     */
    const ssize_t usable = boot_get_area_trailer_status_offset(PARTITION_ID(slot1_partition));

    if (usable < 0) {
        LOG_ERR("no spare image slot on this board: %d", (int) usable);

        return TK_OTA_FAILED;
    }

    if (release.size > (uint32_t) usable) {
        LOG_ERR("the image is %u bytes and the slot holds %u", release.size, (uint32_t) usable);

        return TK_OTA_FAILED;
    }

    if (psa_crypto_init() != PSA_SUCCESS) {
        LOG_ERR("could not start the crypto subsystem");

        return TK_OTA_FAILED;
    }

    hash_op = psa_hash_operation_init();

    if (psa_hash_setup(&hash_op, PSA_ALG_SHA_256) != PSA_SUCCESS) {
        LOG_ERR("could not start hashing");

        return TK_OTA_FAILED;
    }

    if (flash_img_init(&img_ctx) != 0) {
        LOG_ERR("could not open the spare slot for writing");
        (void) psa_hash_abort(&hash_op);

        return TK_OTA_FAILED;
    }

    LOG_INF("fetching %s (%u bytes) — this takes a while", release.url, release.size);

    sink_failed = false;

    const struct tk_fetch_sink image_sink = {on_chunk, nullptr};

    n = tk_fetch(tk_fetch_path_of(release.url), &image_sink, TK_FETCH_IMAGE_TIMEOUT_MS);

    if (n < 0 || sink_failed) {
        (void) psa_hash_abort(&hash_op);

        return TK_OTA_FAILED;
    }

    /* Flushes whatever is left in the staging buffer, padded out to a write
     * block. Without it the tail of the image is still in RAM. */
    if (flash_img_buffered_write(&img_ctx, nullptr, 0, true) != 0) {
        LOG_ERR("could not finish writing the image");
        (void) psa_hash_abort(&hash_op);

        return TK_OTA_FAILED;
    }

    /* First check: the cheapest one. A truncated or padded download is caught
     * before anything is hashed out. */
    if ((uint32_t) n != release.size) {
        LOG_ERR("the image is %d bytes; the manifest said %u", n, release.size);
        (void) psa_hash_abort(&hash_op);

        return TK_OTA_FAILED;
    }

    /* Second: the digest, which is what the signature actually covers. */
    uint8_t digest[tk::kSha256Bytes];
    size_t digest_len = 0;

    if (psa_hash_finish(&hash_op, digest, sizeof(digest), &digest_len) != PSA_SUCCESS ||
        digest_len != sizeof(digest)) {
        LOG_ERR("could not finish hashing the image");

        return TK_OTA_FAILED;
    }

    if (memcmp(digest, release.sha256, sizeof(digest)) != 0) {
        LOG_ERR("the image does not match its digest");

        return TK_OTA_FAILED;
    }

    /* Third: the signature, and the only one that means anything about who
     * produced the image. MCUboot checks its own before booting it. */
    if (!tk::ed25519_verify(release.sig, digest, sizeof(digest), TISCHKARTE_TRUSTED_KEY)) {
        LOG_ERR("the image's signature is not valid — refusing it");

        return TK_OTA_FAILED;
    }

    /*
     * Marked only now, and this is the whole of the commit. Everything above
     * wrote to a slot nothing boots from; this line is what makes the next boot
     * a different one. The same ordering src/sync.cpp uses when it records a
     * corpus version only after the bytes are on disk.
     */
    const int err = boot_request_upgrade(BOOT_UPGRADE_PERMANENT);

    if (err != 0) {
        LOG_ERR("could not mark the image for install: %d", err);

        return TK_OTA_FAILED;
    }

    LOG_INF("firmware %s verified and staged; it installs on the next boot", release.version);

    if (version != nullptr && version_size > 0) {
        (void) snprintf(version, version_size, "%s", release.version);
    }

    return TK_OTA_STAGED;
}
