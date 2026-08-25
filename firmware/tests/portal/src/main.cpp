
#include <string.h>

#include <zephyr/ztest.h>

#include "portal_dns.hpp"
#include "portal_form.hpp"
#include "portal_fsm.hpp"
#include "portal_page.hpp"

using namespace cicala;

namespace
{

constexpr uint32_t kApAddr = 0xC0A80401; // 192.168.4.1

/* Android captive-portal A query for connectivitycheck.gstatic.com. */
const uint8_t kQueryA[] = {
    0x12, 0x34, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 'c',  'o',  'n',
    'n',  'e',  'c',  't',  'i',  'v',  'i',  't',  'y',  'c',  'h',  'e',  'c',  'k',  0x07, 'g',
    's',  't',  'a',  't',  'i',  'c',  0x03, 'c',  'o',  'm',  0x00, 0x00, 0x01, 0x00, 0x01,
};

uint8_t query_aaaa[sizeof(kQueryA)];

uint8_t reply[kDnsMaxMessage];

char page[4096];

bool contains(const char *haystack, const char *needle)
{
    return strstr(haystack, needle) != nullptr;
}

} // namespace

ZTEST_SUITE(cicala_portal, NULL, NULL, NULL, NULL, NULL);

/* DNS */

ZTEST(cicala_portal, test_dns_parses_a_real_query)
{
    DnsQuery q = {};

    zassert_true(dns_parse_query(kQueryA, sizeof(kQueryA), q));
    zassert_equal(q.id, 0x1234);
    zassert_equal(q.qtype, kDnsTypeA);
    zassert_equal(q.qclass, kDnsClassIn);
    zassert_true(q.recursion_desired);
    zassert_equal(q.name_offset, 12);
    zassert_equal(q.question_end, sizeof(kQueryA));
}

ZTEST(cicala_portal, test_dns_answers_with_the_portal_address)
{
    const uint16_t n = dns_hijack(kQueryA, sizeof(kQueryA), kApAddr, reply, sizeof(reply));

    zassert_equal(n, sizeof(kQueryA) + kDnsAnswerBytes);

    // Echo the transaction ID.
    zassert_equal(reply[0], 0x12);
    zassert_equal(reply[1], 0x34);

    // Mark a successful authoritative response and echo recursion desired.
    zassert_equal(reply[2] & 0x80, 0x80, "the reply must say it is a response");
    zassert_equal(reply[2] & 0x04, 0x04, "and that it is authoritative");
    zassert_equal(reply[2] & 0x01, 0x01, "RD is echoed back");
    zassert_equal(reply[3], 0x00, "no recursion available, and NOERROR");

    // One question and one answer.
    zassert_equal(reply[5], 1);
    zassert_equal(reply[7], 1);
    zassert_equal(reply[9], 0);
    zassert_equal(reply[11], 0);

    const uint8_t *answer = &reply[sizeof(kQueryA)];

    // Point the answer name at the query name and prevent caching.
    zassert_equal(answer[0], 0xC0);
    zassert_equal(answer[1], 12);
    zassert_equal(answer[3], kDnsTypeA);
    zassert_equal(answer[5], kDnsClassIn);

    zassert_equal(answer[6], 0);
    zassert_equal(answer[7], 0);
    zassert_equal(answer[8], 0);
    zassert_equal(answer[9], 0);

    zassert_equal(answer[11], 4, "an A record's rdata is four bytes");
    zassert_equal(answer[12], 192);
    zassert_equal(answer[13], 168);
    zassert_equal(answer[14], 4);
    zassert_equal(answer[15], 1);
}

ZTEST(cicala_portal, test_dns_answers_aaaa_empty_rather_than_not_at_all)
{
    memcpy(query_aaaa, kQueryA, sizeof(kQueryA));
    query_aaaa[sizeof(kQueryA) - 3] = kDnsTypeAaaa;

    const uint16_t n = dns_hijack(query_aaaa, sizeof(query_aaaa), kApAddr, reply, sizeof(reply));

    zassert_equal(n, sizeof(kQueryA), "the question comes back with no answer after it");
    zassert_equal(reply[3], 0x00, "and NOERROR, so the client stops asking");
    zassert_equal(reply[7], 0, "ANCOUNT is zero");
}

ZTEST(cicala_portal, test_dns_rejects_what_it_should_not_answer)
{
    DnsQuery q = {};
    uint8_t bad[sizeof(kQueryA)];

    zassert_false(dns_parse_query(kQueryA, 11, q), "shorter than a header");
    zassert_false(dns_parse_query(nullptr, sizeof(kQueryA), q));

    // A response.
    memcpy(bad, kQueryA, sizeof(bad));
    bad[2] |= 0x80;
    zassert_false(dns_parse_query(bad, sizeof(bad), q));

    // A nonzero opcode.
    memcpy(bad, kQueryA, sizeof(bad));
    bad[2] |= 0x08;
    zassert_false(dns_parse_query(bad, sizeof(bad), q));

    // More than one question.
    memcpy(bad, kQueryA, sizeof(bad));
    bad[5] = 2;
    zassert_false(dns_parse_query(bad, sizeof(bad), q));

    // A compressed query name.
    memcpy(bad, kQueryA, sizeof(bad));
    bad[12] = 0xC0;
    zassert_false(dns_parse_query(bad, sizeof(bad), q));

    // A label extending past the packet.
    memcpy(bad, kQueryA, sizeof(bad));
    bad[12] = 0x3F;
    zassert_false(dns_parse_query(bad, sizeof(bad), q));

    zassert_false(dns_parse_query(kQueryA, sizeof(kQueryA) - 2, q));
}

ZTEST(cicala_portal, test_dns_will_not_overrun_the_reply_buffer)
{
    DnsQuery q = {};

    zassert_true(dns_parse_query(kQueryA, sizeof(kQueryA), q));

    const uint16_t n = dns_build_reply(kQueryA, sizeof(kQueryA), q, kApAddr, reply,
                                       sizeof(kQueryA) + kDnsAnswerBytes - 1);

    zassert_equal(n, 0);
}

/* Form decoding */

ZTEST(cicala_portal, test_form_reads_the_fields_a_browser_posts)
{
    const char body[] = "ssid=Cafe+Krone&psk=hunter2&lang=de";
    char value[kPskBufSize];

    zassert_equal(form_field(body, sizeof(body) - 1, "ssid", value, sizeof(value)), 10);
    zassert_str_equal(value, "Cafe Krone", "a plus is a space");

    zassert_equal(form_field(body, sizeof(body) - 1, "psk", value, sizeof(value)), 7);
    zassert_str_equal(value, "hunter2");

    zassert_equal(form_field(body, sizeof(body) - 1, "lang", value, sizeof(value)), 2);
    zassert_str_equal(value, "de", "the last field has no ampersand after it");
}

ZTEST(cicala_portal, test_form_decodes_a_password_worth_escaping)
{
    const char body[] = "psk=a%26b%3Dc%25d+e%2Bf";
    char value[kPskBufSize];

    zassert_equal(form_field(body, sizeof(body) - 1, "psk", value, sizeof(value)), 11);
    zassert_str_equal(value, "a&b=c%d e+f");
}

ZTEST(cicala_portal, test_form_matches_whole_keys_only)
{
    const char body[] = "xssid=wrong&ssid_hidden=wrong&ssid=right";
    char value[kSsidBufSize];

    zassert_equal(form_field(body, sizeof(body) - 1, "ssid", value, sizeof(value)), 5);
    zassert_str_equal(value, "right");
}

ZTEST(cicala_portal, test_form_rejects_rather_than_guesses)
{
    char value[kPskBufSize];

    zassert_equal(form_field("psk=ab%", 7, "psk", value, sizeof(value)), -1);
    zassert_equal(form_field("psk=ab%4", 8, "psk", value, sizeof(value)), -1);

    zassert_equal(form_field("psk=ab%zz", 9, "psk", value, sizeof(value)), -1);

    zassert_equal(form_field("ssid=x", 6, "psk", value, sizeof(value)), -1);
    zassert_equal(form_field("psk", 3, "psk", value, sizeof(value)), -1);

    char small[4];
    zassert_equal(form_field("psk=abcdef", 10, "psk", small, sizeof(small)), -1);
}

ZTEST(cicala_portal, test_form_accepts_an_empty_value)
{
    char value[kPskBufSize];

    zassert_equal(form_field("ssid=x&psk=&lang=en", 19, "psk", value, sizeof(value)), 0);
    zassert_str_equal(value, "");
}

/* Page rendering */

ZTEST(cicala_portal, test_page_escapes_a_network_name_from_the_air)
{
    ScanEntry nets[1] = {};

    strcpy(nets[0].ssid, "<script>alert(1)</script>");
    nets[0].secure = true;

    const int n = page_setup(page, sizeof(page), nets, 1, "en");

    zassert_true(n > 0, "the page has to fit");
    zassert_false(contains(page, "<script>"), "the tag must not survive into the page");
    zassert_true(contains(page, "&lt;script&gt;alert(1)&lt;/script&gt;"));
}

ZTEST(cicala_portal, test_page_escapes_a_name_that_would_break_out_of_an_attribute)
{
    ScanEntry nets[1] = {};

    strcpy(nets[0].ssid, "a\" onfocus=\"x");
    nets[0].secure = true;

    zassert_true(page_setup(page, sizeof(page), nets, 1, "en") > 0);
    zassert_false(contains(page, "onfocus=\"x\""));
    zassert_true(contains(page, "&quot; onfocus=&quot;x"));
}

ZTEST(cicala_portal, test_page_setup_lists_networks_and_marks_the_language)
{
    ScanEntry nets[2] = {};

    strcpy(nets[0].ssid, "Krone");
    nets[0].secure = true;
    strcpy(nets[1].ssid, "Gastzugang");
    nets[1].secure = false;

    zassert_true(page_setup(page, sizeof(page), nets, 2, "de") > 0);

    zassert_true(contains(page, "Krone"));
    zassert_true(contains(page, "Gastzugang (open)"), "an open network says so");
    zassert_true(contains(page, "<option value=\"de\" selected>"));
    zassert_false(contains(page, "<option value=\"en\" selected>"));
    zassert_true(contains(page, "type=\"password\""));
    zassert_true(contains(page, "<span class=\"wordmark\">cicala</span>"));
    zassert_false(contains(page, "class=\"mark\""), "the old KV tile must not return");
    zassert_true(contains(page, "background:#faf8f2"));
    zassert_true(contains(page, "color:#1f1f1d"));
    zassert_true(contains(page, "#c24a22"), "rust is reserved for interactive states");
    zassert_false(contains(page, "http://"), "the captive page must not fetch remote assets");
    zassert_false(contains(page, "https://"), "the captive page must not fetch remote assets");
}

ZTEST(cicala_portal, test_page_setup_still_takes_a_name_when_the_scan_found_nothing)
{
    zassert_true(page_setup(page, sizeof(page), nullptr, 0, "en") > 0);

    zassert_true(contains(page, "name=\"ssid\""));
    zassert_true(contains(page, "No networks in range"));
}

ZTEST(cicala_portal, test_page_status_never_shows_the_saved_password)
{
    const PortalStatus status = {
        "Cicala-A1B2",
        "esp32s3_devkitc",
        "en",
        "2026.07.2",
        240,
        "0.1.0",
        "Cafe Krone",
        true,
        "192.168.1.44",
        "",
        "Questions are up to date.",
        287,
    };

    zassert_true(page_status(page, sizeof(page), status) > 0);

    zassert_true(contains(page, "Cicala-A1B2"));
    zassert_true(contains(page, "esp32s3_devkitc"));
    zassert_true(contains(page, "240 in en"));
    zassert_true(contains(page, "2026.07.2"));
    zassert_true(contains(page, "0.1.0"));
    zassert_true(contains(page, "Cafe Krone"));
    zassert_true(contains(page, "Connected"));
    zassert_true(contains(page, "192.168.1.44"));
    zassert_true(contains(page, "287 seconds left"));
    zassert_true(contains(page, "Questions are up to date."));
}

ZTEST(cicala_portal, test_page_status_reports_an_unconfigured_device)
{
    const PortalStatus status = {
        "Cicala-A1B2",
        "esp32s3_devkitc",
        "en",
        "2026.07.2",
        240,
        "0.1.0",
        nullptr,
        false,
        "",
        "Wi-Fi error -5",
        "",
        12,
    };

    zassert_true(page_status(page, sizeof(page), status) > 0);
    zassert_true(contains(page, "none"), "no saved network says so plainly");
    zassert_true(contains(page, "Offline"));
    zassert_true(contains(page, "Wi-Fi error -5"));
}

ZTEST(cicala_portal, test_page_notice_escapes_its_message)
{
    zassert_true(page_notice(page, sizeof(page), "Done", "Use <network> & save") > 0);
    zassert_true(contains(page, "Use &lt;network&gt; &amp; save"));
}

ZTEST(cicala_portal, test_page_forget_requires_confirmation)
{
    zassert_true(page_forget_confirm(page, sizeof(page)) > 0);
    zassert_true(contains(page, "Confirm and forget Wi-Fi"));
    zassert_true(contains(page, "Your questions and language stay intact."));
}

ZTEST(cicala_portal, test_page_reports_a_buffer_too_small_rather_than_writing_past_it)
{
    ScanEntry nets[1] = {};
    char tiny[64];

    strcpy(nets[0].ssid, "Krone");

    zassert_equal(page_setup(tiny, sizeof(tiny), nets, 1, "en"), -1);
    zassert_equal(page_saved(tiny, sizeof(tiny), "Krone"), -1);
}

ZTEST(cicala_portal, test_page_saved_names_the_network_being_joined)
{
    zassert_true(page_saved(page, sizeof(page), "Cafe & Bar") > 0);
    zassert_true(contains(page, "Cafe &amp; Bar"));
}

ZTEST(cicala_portal, test_html_escape_covers_every_character_that_matters)
{
    char out[64];
    const char in[] = "&<>\"'";

    zassert_equal(html_escape(in, sizeof(in) - 1, out, sizeof(out)), 24);
    zassert_str_equal(out, "&amp;&lt;&gt;&quot;&#39;");
}

ZTEST(cicala_portal, test_html_escape_reports_a_buffer_too_small)
{
    char out[8];

    zassert_equal(html_escape("&&&&", 4, out, sizeof(out)), -1);
}

/* State machine */

namespace
{

using State = PortalFsm::State;

class FakeIo : public PortalIo
{
public:
    int scans = 0;
    int ap_starts = 0;
    int serve_starts = 0;
    int connects = 0;
    int teardowns = 0;
    int cards = 0;
    PortalCard last_card = PortalCard::SETUP;

    bool scan_succeeds = true;
    bool ap_succeeds = true;
    bool serve_succeeds = true;
    bool connect_succeeds = true;

    bool scan_start() override
    {
        scans++;
        return scan_succeeds;
    }

    bool ap_start() override
    {
        ap_starts++;
        return ap_succeeds;
    }

    bool serve_start() override
    {
        serve_starts++;
        return serve_succeeds;
    }

    bool connect_start() override
    {
        connects++;
        return connect_succeeds;
    }

    void teardown() override { teardowns++; }

    void show(PortalCard card) override
    {
        cards++;
        last_card = card;
    }
};

class TestPortalFsm : public PortalFsm
{
public:
    using PortalFsm::PortalFsm;

    int64_t clock = 0;

    void advance(int64_t ms) { clock += ms; }

protected:
    int64_t now_ms() const override { return clock; }
};

void settle(TestPortalFsm &fsm)
{
    for (int i = 0; i < 16; i++) {
        const int before = fsm.get_current_state();

        fsm.run();

        if (fsm.get_current_state() == before) {
            return;
        }
    }
}

void reach_serving(TestPortalFsm &fsm, FakeIo &io)
{
    fsm.post_start();
    settle(fsm);

    fsm.post_scan_done();
    settle(fsm);

    fsm.post_ap_ready(true);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(SERVING));
    zassert_equal(io.serve_starts, 1);
}

} // namespace

ZTEST(cicala_portal, test_fsm_starts_off_and_stays_there)
{
    FakeIo io;
    TestPortalFsm fsm(io);

    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(OFF));
    zassert_false(fsm.is_active(), "nothing is on air until somebody asks");
    zassert_equal(io.scans, 0);
    zassert_equal(io.ap_starts, 0);
}

ZTEST(cicala_portal, test_fsm_scans_before_it_raises_the_access_point)
{
    FakeIo io;
    TestPortalFsm fsm(io);

    fsm.post_start();
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(SCANNING));
    zassert_equal(io.scans, 1);
    zassert_equal(io.ap_starts, 0, "the access point must not be up during a scan");

    fsm.post_scan_done();
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(AP_STARTING));
    zassert_equal(io.ap_starts, 1);
    zassert_equal(io.serve_starts, 0, "nothing is served until the AP reports itself up");

    fsm.post_ap_ready(true);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(SERVING));
    zassert_equal(io.serve_starts, 1);
    zassert_equal(io.last_card, PortalCard::SETUP, "the panel names the network to join");
}

ZTEST(cicala_portal, test_fsm_gives_up_on_a_scan_that_never_reports)
{
    FakeIo io;
    TestPortalFsm fsm(io);

    fsm.post_start();
    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(SCANNING));

    fsm.advance(kPortalScanMs - 1);
    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(SCANNING), "not yet");

    fsm.advance(1);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(AP_STARTING));
}

ZTEST(cicala_portal, test_fsm_carries_on_when_the_scan_cannot_be_started)
{
    FakeIo io;
    TestPortalFsm fsm(io);

    io.scan_succeeds = false;

    fsm.post_start();
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(AP_STARTING),
                  "no waiting for a scan that is not running");
}

ZTEST(cicala_portal, test_fsm_ends_when_the_access_point_will_not_come_up)
{
    FakeIo io;
    TestPortalFsm fsm(io);

    fsm.post_start();
    settle(fsm);
    fsm.post_scan_done();
    settle(fsm);

    fsm.post_ap_ready(false);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(OFF));
    zassert_equal(io.teardowns, 1, "whatever came up has to come down again");
    zassert_equal(io.serve_starts, 0);
    zassert_false(fsm.is_active());
}

ZTEST(cicala_portal, test_fsm_ends_when_nothing_can_be_served)
{
    FakeIo io;
    TestPortalFsm fsm(io);

    io.serve_succeeds = false;

    fsm.post_start();
    settle(fsm);
    fsm.post_scan_done();
    settle(fsm);
    fsm.post_ap_ready(true);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(OFF));
    zassert_equal(io.teardowns, 1);
}

ZTEST(cicala_portal, test_fsm_joins_a_network_when_the_form_arrives)
{
    FakeIo io;
    TestPortalFsm fsm(io);

    reach_serving(fsm, io);

    fsm.post_credentials();
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(CONNECTING));
    zassert_equal(io.connects, 1);
    zassert_equal(io.last_card, PortalCard::CONNECTING,
                  "the panel reports it, because joining knocks the phone off");

    fsm.post_connected(true);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(CONNECTED));
    zassert_true(fsm.is_connected());
    zassert_equal(io.last_card, PortalCard::CONNECTED);
    zassert_equal(io.teardowns, 0, "the AP stays up so the status page can be read");
}

ZTEST(cicala_portal, test_fsm_puts_a_refused_password_back_on_the_form)
{
    FakeIo io;
    TestPortalFsm fsm(io);

    reach_serving(fsm, io);

    fsm.post_credentials();
    settle(fsm);

    fsm.post_connected(false);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(SERVING));
    zassert_equal(io.last_card, PortalCard::REFUSED);
    zassert_equal(io.serve_starts, 1, "the access point never went down, so nothing restarts");
    zassert_equal(io.ap_starts, 1);

    fsm.post_credentials();
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(CONNECTING));
    zassert_equal(io.connects, 2);
}

ZTEST(cicala_portal, test_fsm_treats_a_silent_network_as_a_refusal)
{
    FakeIo io;
    TestPortalFsm fsm(io);

    reach_serving(fsm, io);

    fsm.post_credentials();
    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(CONNECTING));

    fsm.advance(kConnectTimeoutMs - 1);
    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(CONNECTING));

    fsm.advance(1);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(SERVING));
    zassert_equal(io.last_card, PortalCard::REFUSED);
}

ZTEST(cicala_portal, test_fsm_closes_the_window_on_its_own)
{
    FakeIo io;
    TestPortalFsm fsm(io);

    reach_serving(fsm, io);

    fsm.advance(kPortalWindowMs - 1);
    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(SERVING));

    fsm.advance(1);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(OFF));
    zassert_equal(io.teardowns, 1);
    zassert_false(fsm.is_active());
}

ZTEST(cicala_portal, test_fsm_gives_the_status_page_a_window_of_its_own)
{
    FakeIo io;
    TestPortalFsm fsm(io);

    reach_serving(fsm, io);

    fsm.advance(kPortalWindowMs - 10);
    settle(fsm);

    fsm.post_credentials();
    settle(fsm);
    fsm.post_connected(true);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(CONNECTED));

    fsm.advance(kPortalWindowMs - 1);
    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(CONNECTED));

    fsm.advance(1);
    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(OFF));
    zassert_equal(io.teardowns, 1);
}

ZTEST(cicala_portal, test_fsm_stops_from_wherever_it_is)
{
    for (int stop_at = 0; stop_at < 4; stop_at++) {
        FakeIo io;
        TestPortalFsm fsm(io);

        fsm.post_start();
        settle(fsm);

        if (stop_at >= 1) {
            fsm.post_scan_done();
            settle(fsm);
        }

        if (stop_at >= 2) {
            fsm.post_ap_ready(true);
            settle(fsm);
        }

        if (stop_at >= 3) {
            fsm.post_credentials();
            settle(fsm);
        }

        fsm.post_stop();
        settle(fsm);

        zassert_equal(fsm.get_current_state(), STATE(OFF), "stop at step %d", stop_at);
        zassert_equal(io.teardowns, 1, "stop at step %d", stop_at);
        zassert_false(fsm.is_active());
    }
}

ZTEST(cicala_portal, test_fsm_ignores_a_stop_that_arrives_with_nothing_running)
{
    FakeIo io;
    TestPortalFsm fsm(io);

    fsm.post_stop();
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(OFF));
    zassert_equal(io.teardowns, 0);

    fsm.post_start();
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(SCANNING));
}
