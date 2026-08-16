/* Download and validate a firmware image. */

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

LOG_MODULE_REGISTER(kveld_ota, LOG_LEVEL_INF);

/* A firmware manifest is one flat object of six fields. */
#define MANIFEST_MAX 1024

static char manifest_buf[MANIFEST_MAX];

/* What the image is written through, and what the running hash is kept in. */
static struct flash_img_context img_ctx;
static psa_hash_operation_t hash_op;
static bool sink_failed;

const char *kveld_ota_running_version(void)
{
    return APP_VERSION_STRING;
}

/* One chunk: into the slot, and into the digest. */
static int on_chunk(void *ctx, const uint8_t *data, size_t len)
{
    ARG_UNUSED(ctx);

    if (psa_hash_update(&hash_op, data, len) != PSA_SUCCESS) {
        LOG_ERR("could not hash the image");
        sink_failed = true;

        return -EIO;
    }

    /* flash_img pads the final chunk when flash_img_buffered_write() is flushed. */
    const int err = flash_img_buffered_write(&img_ctx, data, len, false);

    if (err != 0) {
        LOG_ERR("could not write to the spare slot: %d", err);
        sink_failed = true;

        return err;
    }

    return 0;
}

enum kveld_ota_result kveld_ota_run(char *version, size_t version_size)
{
    kveld::FirmwareRelease release = {};

    LOG_INF("checking %s for firmware newer than %s", CONFIG_KVELD_OTA_BASE_URL,
            APP_VERSION_STRING);

    struct kveld_fetch_mem mem = {};

    mem.buf = (uint8_t *) manifest_buf;
    mem.capacity = sizeof(manifest_buf);

    const struct kveld_fetch_sink manifest_sink = {kveld_fetch_mem_write, &mem};

    int n = kveld_fetch(kveld_fetch_path_of(CONFIG_KVELD_OTA_BASE_URL "/firmware.json"),
                        &manifest_sink, KVELD_FETCH_TIMEOUT_MS);

    if (n < 0) {
        return KVELD_OTA_FAILED;
    }

    if (!kveld::firmware_parse(manifest_buf, (size_t) n, release)) {
        LOG_ERR("the firmware manifest did not parse");

        return KVELD_OTA_FAILED;
    }

    if (release.schema != kveld::kFirmwareSchema) {
        /* Unknown schemas may change field meaning. */
        LOG_ERR("firmware manifest schema %u, expected %u", release.schema, kveld::kFirmwareSchema);

        return KVELD_OTA_FAILED;
    }

    /* Reject signed manifests at or below the installed version. */
    if (kveld::version_compare(release.version, APP_VERSION_STRING) <= 0) {
        LOG_INF("running %s and the manifest offers %s — nothing to do", APP_VERSION_STRING,
                release.version);

        return KVELD_OTA_CURRENT;
    }

    if (!release.signed_) {
        /* An image built without the manifest key. */
        LOG_ERR("refusing an unsigned firmware release");

        return KVELD_OTA_FAILED;
    }

    /* Check the secondary slot size before downloading. */
    /* Not the slot's size: the usable size. */
    const ssize_t usable = boot_get_area_trailer_status_offset(PARTITION_ID(slot1_partition));

    if (usable < 0) {
        LOG_ERR("no spare image slot on this board: %d", (int) usable);

        return KVELD_OTA_FAILED;
    }

    if (release.size > (uint32_t) usable) {
        LOG_ERR("the image is %u bytes and the slot holds %u", release.size, (uint32_t) usable);

        return KVELD_OTA_FAILED;
    }

    if (psa_crypto_init() != PSA_SUCCESS) {
        LOG_ERR("could not start the crypto subsystem");

        return KVELD_OTA_FAILED;
    }

    hash_op = psa_hash_operation_init();

    if (psa_hash_setup(&hash_op, PSA_ALG_SHA_256) != PSA_SUCCESS) {
        LOG_ERR("could not start hashing");

        return KVELD_OTA_FAILED;
    }

    if (flash_img_init(&img_ctx) != 0) {
        LOG_ERR("could not open the spare slot for writing");
        (void) psa_hash_abort(&hash_op);

        return KVELD_OTA_FAILED;
    }

    LOG_INF("fetching %s (%u bytes) — this takes a while", release.url, release.size);

    sink_failed = false;

    const struct kveld_fetch_sink image_sink = {on_chunk, nullptr};

    n = kveld_fetch(kveld_fetch_path_of(release.url), &image_sink, KVELD_FETCH_IMAGE_TIMEOUT_MS);

    if (n < 0 || sink_failed) {
        (void) psa_hash_abort(&hash_op);

        return KVELD_OTA_FAILED;
    }

    /* Flushes whatever is left in the staging buffer, padded out to a write block. */
    if (flash_img_buffered_write(&img_ctx, nullptr, 0, true) != 0) {
        LOG_ERR("could not finish writing the image");
        (void) psa_hash_abort(&hash_op);

        return KVELD_OTA_FAILED;
    }

    if ((uint32_t) n != release.size) {
        LOG_ERR("the image is %d bytes; the manifest said %u", n, release.size);
        (void) psa_hash_abort(&hash_op);

        return KVELD_OTA_FAILED;
    }

    uint8_t digest[kveld::kSha256Bytes];
    size_t digest_len = 0;

    if (psa_hash_finish(&hash_op, digest, sizeof(digest), &digest_len) != PSA_SUCCESS ||
        digest_len != sizeof(digest)) {
        LOG_ERR("could not finish hashing the image");

        return KVELD_OTA_FAILED;
    }

    if (memcmp(digest, release.sha256, sizeof(digest)) != 0) {
        LOG_ERR("the image does not match its digest");

        return KVELD_OTA_FAILED;
    }

    if (!kveld::ed25519_verify(release.sig, digest, sizeof(digest), KVELD_TRUSTED_KEY)) {
        LOG_ERR("the image's signature is not valid — refusing it");

        return KVELD_OTA_FAILED;
    }

    /* Commit only after the complete image has passed validation. */
    const int err = boot_request_upgrade(BOOT_UPGRADE_PERMANENT);

    if (err != 0) {
        LOG_ERR("could not mark the image for install: %d", err);

        return KVELD_OTA_FAILED;
    }

    LOG_INF("firmware %s verified and staged; it installs on the next boot", release.version);

    if (version != nullptr && version_size > 0) {
        (void) snprintf(version, version_size, "%s", release.version);
    }

    return KVELD_OTA_STAGED;
}
