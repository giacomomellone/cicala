
#include <string.h>

#include <zephyr/ztest.h>

#include "manifest.hpp"
#include "release_manifest.h"

using namespace tk;

namespace
{

constexpr size_t kLen = sizeof(kReleaseManifest) - 1;

} // namespace

ZTEST_SUITE(tk_sync, NULL, NULL, NULL, NULL, NULL);

ZTEST(tk_sync, test_versions_compare_as_numbers_not_as_text)
{
    zassert_true(version_compare("2026.08.9", "2026.08.10") < 0);
    zassert_true(version_compare("2026.08.10", "2026.08.9") > 0);

    zassert_true(version_compare("2026.07.2", "2026.08.1") < 0);
    zassert_true(version_compare("2025.12.1", "2026.01.1") < 0);
    zassert_equal(version_compare("2026.08.1", "2026.08.1"), 0);
}

ZTEST(tk_sync, test_a_missing_component_counts_as_zero)
{
    zassert_equal(version_compare("2026.08", "2026.08.0"), 0);
    zassert_true(version_compare("2026.08", "2026.08.1") < 0);
    zassert_true(version_compare("2026.08.1", "2026.08") > 0);
}

ZTEST(tk_sync, test_a_version_long_enough_to_overflow_does_not_wrap)
{
    zassert_true(version_compare("999999999999999999", "2026.08.1") > 0);
    zassert_equal(version_compare(nullptr, "2026.08.1"), 0);
}

ZTEST(tk_sync, test_the_published_manifest_parses)
{
    Manifest m = {};

    zassert_true(manifest_parse(kReleaseManifest, kLen, m));
    zassert_equal(m.schema, kManifestSchema, "the shipped manifest is schema %u", m.schema);
    zassert_str_equal(m.version, "2026.08.1");
    zassert_true(m.min_fw[0] != '\0');
}

ZTEST(tk_sync, test_the_published_entry_carries_a_signature_and_a_raw_bundle)
{
    ManifestEntry e = {};

    zassert_true(manifest_entry(kReleaseManifest, kLen, "en", e));
    zassert_true(e.signed_, "the release was signed, so this must not read as a dev build");
    zassert_true(e.size > 0);
    zassert_true(e.count > 0);

    const size_t url_len = strlen(e.url);
    zassert_true(url_len > 4);
    zassert_str_equal(e.url + url_len - 4, ".qdb");

    ManifestEntry de = {};
    zassert_true(manifest_entry(kReleaseManifest, kLen, "de", de));
    zassert_true(de.size != e.size || de.count != e.count);
}

ZTEST(tk_sync, test_a_language_that_is_not_published_is_not_invented)
{
    ManifestEntry e = {};

    zassert_false(manifest_entry(kReleaseManifest, kLen, "fr", e));
}

ZTEST(tk_sync, test_an_unsigned_development_bundle_is_marked_as_such)
{
    static const char json[] =
        "{\"schema\":3,\"version\":\"dev\",\"min_fw\":\"0.1.0\",\"languages\":{\"en\":{"
        "\"url\":\"http://h/b.qdb\",\"size\":10,"
        "\"sha256\":\"0000000000000000000000000000000000000000000000000000000000000000\","
        "\"sig\":null,\"count\":1}}}";

    ManifestEntry e = {};

    zassert_true(manifest_entry(json, sizeof(json) - 1, "en", e));
    zassert_false(e.signed_, "an unsigned bundle must never read as signed");
}

ZTEST(tk_sync, test_a_truncated_or_empty_manifest_is_refused)
{
    Manifest m = {};

    zassert_false(manifest_parse(nullptr, 10, m));
    zassert_false(manifest_parse(kReleaseManifest, 0, m));

    for (size_t cut = 1; cut < kLen; cut += 16) {
        Manifest partial = {};
        ManifestEntry e = {};

        (void) manifest_parse(kReleaseManifest, cut, partial);
        (void) manifest_entry(kReleaseManifest, cut, "en", e);
    }
}

ZTEST(tk_sync, test_a_field_too_long_to_hold_is_refused)
{
    static char json[1024];
    int n = snprintk(json, sizeof(json),
                     "{\"schema\":3,\"version\":\"1\",\"min_fw\":\"0\",\"languages\":{\"en\":{"
                     "\"url\":\"http://h/");

    for (size_t i = 0; i < kMaxUrlBytes + 16 && n < (int) sizeof(json) - 8; i++) {
        json[n] = 'x';
        n++;
    }

    n += snprintk(json + n, sizeof(json) - n,
                  "\",\"size\":1,\"sha256\":\"00\",\"sig\":null,\"count\":1}}}");

    ManifestEntry e = {};

    zassert_false(manifest_entry(json, (size_t) n, "en", e));
}

ZTEST(tk_sync, test_a_malformed_digest_or_signature_is_refused)
{
    ManifestEntry e = {};

    static const char short_hex[] =
        "{\"schema\":3,\"version\":\"1\",\"min_fw\":\"0\",\"languages\":{\"en\":{"
        "\"url\":\"http://h/b.qdb\",\"size\":1,\"sha256\":\"abcd\",\"sig\":null,\"count\":1}}}";
    zassert_false(manifest_entry(short_hex, sizeof(short_hex) - 1, "en", e));

    static const char bad_hex[] =
        "{\"schema\":3,\"version\":\"1\",\"min_fw\":\"0\",\"languages\":{\"en\":{"
        "\"url\":\"http://h/b.qdb\",\"size\":1,"
        "\"sha256\":\"zz00000000000000000000000000000000000000000000000000000000000000\","
        "\"sig\":null,\"count\":1}}}";
    zassert_false(manifest_entry(bad_hex, sizeof(bad_hex) - 1, "en", e));

    static const char short_sig[] =
        "{\"schema\":3,\"version\":\"1\",\"min_fw\":\"0\",\"languages\":{\"en\":{"
        "\"url\":\"http://h/b.qdb\",\"size\":1,"
        "\"sha256\":\"0000000000000000000000000000000000000000000000000000000000000000\","
        "\"sig\":\"YWJj\",\"count\":1}}}";
    zassert_false(manifest_entry(short_sig, sizeof(short_sig) - 1, "en", e));
}

ZTEST(tk_sync, test_a_zero_sized_bundle_is_refused)
{
    static const char json[] =
        "{\"schema\":3,\"version\":\"1\",\"min_fw\":\"0\",\"languages\":{\"en\":{"
        "\"url\":\"http://h/b.qdb\",\"size\":0,"
        "\"sha256\":\"0000000000000000000000000000000000000000000000000000000000000000\","
        "\"sig\":null,\"count\":1}}}";

    ManifestEntry e = {};

    zassert_false(manifest_entry(json, sizeof(json) - 1, "en", e));
}

ZTEST(tk_sync, test_a_firmware_manifest_parses)
{
    static const char json[] =
        "{\"schema\":1,\"version\":\"0.2.0\","
        "\"url\":\"https://h/device/tischkarte-0.2.0.bin\",\"size\":782628,"
        "\"sha256\":\"0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20\","
        "\"sig\":\"" /* 64 bytes of 0x41, base64 */
        "QUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQQ=="
        "\"}";

    FirmwareRelease r = {};

    zassert_true(firmware_parse(json, sizeof(json) - 1, r));
    zassert_equal(r.schema, 1);
    zassert_str_equal(r.version, "0.2.0");
    zassert_str_equal(r.url, "https://h/device/tischkarte-0.2.0.bin");
    zassert_equal(r.size, 782628);
    zassert_true(r.signed_);
    zassert_equal(r.sha256[0], 0x01);
    zassert_equal(r.sha256[31], 0x20);
    zassert_equal(r.sig[0], 'A');
    zassert_equal(r.sig[63], 'A');
}

ZTEST(tk_sync, test_an_unsigned_firmware_manifest_parses_but_is_marked)
{
    static const char json[] =
        "{\"schema\":1,\"version\":\"0.2.0\",\"url\":\"https://h/f.bin\",\"size\":1,"
        "\"sha256\":\"0000000000000000000000000000000000000000000000000000000000000000\","
        "\"sig\":null}";

    FirmwareRelease r = {};

    zassert_true(firmware_parse(json, sizeof(json) - 1, r));
    zassert_false(r.signed_);
}

ZTEST(tk_sync, test_a_firmware_manifest_missing_a_field_is_refused)
{
    FirmwareRelease r = {};

    static const char no_version[] =
        "{\"schema\":1,\"url\":\"https://h/f.bin\",\"size\":1,"
        "\"sha256\":\"0000000000000000000000000000000000000000000000000000000000000000\","
        "\"sig\":null}";
    zassert_false(firmware_parse(no_version, sizeof(no_version) - 1, r));

    static const char no_url[] =
        "{\"schema\":1,\"version\":\"0.2.0\",\"size\":1,"
        "\"sha256\":\"0000000000000000000000000000000000000000000000000000000000000000\","
        "\"sig\":null}";
    zassert_false(firmware_parse(no_url, sizeof(no_url) - 1, r));

    static const char no_digest[] =
        "{\"schema\":1,\"version\":\"0.2.0\",\"url\":\"https://h/f.bin\",\"size\":1,"
        "\"sig\":null}";
    zassert_false(firmware_parse(no_digest, sizeof(no_digest) - 1, r));
}

ZTEST(tk_sync, test_a_zero_sized_image_is_refused)
{
    static const char json[] =
        "{\"schema\":1,\"version\":\"0.2.0\",\"url\":\"https://h/f.bin\",\"size\":0,"
        "\"sha256\":\"0000000000000000000000000000000000000000000000000000000000000000\","
        "\"sig\":null}";

    FirmwareRelease r = {};

    zassert_false(firmware_parse(json, sizeof(json) - 1, r));
}

ZTEST(tk_sync, test_firmware_versions_order_the_way_ota_needs)
{
    zassert_true(version_compare("0.2.0", "0.1.0") > 0);
    zassert_equal(version_compare("0.1.0", "0.1.0"), 0);
    zassert_true(version_compare("0.9.0", "0.10.0") < 0);

    zassert_equal(version_compare("0.1.0", "0.1.0+0"), 0);
}
