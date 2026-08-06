/*
 * The parsing and rendering half of the setup portal.
 *
 * Everything here reads something written by somebody else — a DNS query from a
 * phone, a form body from a browser, a network name from the air — so most of
 * these cases are about what happens when that input is wrong. The device is
 * the side reading packets off a network anyone within range can join.
 */

#include <string.h>

#include <zephyr/ztest.h>

#include "portal_dns.hpp"
#include "portal_form.hpp"
#include "portal_fsm.hpp"
#include "portal_page.hpp"

using namespace tk;

namespace
{

/** 192.168.4.1, the address the portal answers with. */
constexpr uint32_t kApAddr = 0xC0A80401;

/*
 * A real query for `connectivitycheck.gstatic.com`, which is the name Android
 * asks for when it is deciding whether it is behind a captive portal.
 *
 * Header: id 0x1234, flags RD, one question. Then the labels, then A / IN.
 */
const uint8_t kQueryA[] = {
    0x12, 0x34, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 'c',  'o',  'n',
    'n',  'e',  'c',  't',  'i',  'v',  'i',  't',  'y',  'c',  'h',  'e',  'c',  'k',  0x07, 'g',
    's',  't',  'a',  't',  'i',  'c',  0x03, 'c',  'o',  'm',  0x00, 0x00, 0x01, 0x00, 0x01,
};

/** The same name, asked as AAAA — which every phone sends alongside the A. */
uint8_t query_aaaa[sizeof(kQueryA)];

uint8_t reply[kDnsMaxMessage];

/** Pages are built here rather than on the stack; the status page is smallest. */
char page[4096];

bool contains(const char *haystack, const char *needle)
{
    return strstr(haystack, needle) != nullptr;
}

} // namespace

ZTEST_SUITE(tk_portal, NULL, NULL, NULL, NULL, NULL);

/* ------------------------------------------------------------------- DNS */

ZTEST(tk_portal, test_dns_parses_a_real_query)
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

ZTEST(tk_portal, test_dns_answers_with_the_portal_address)
{
    const uint16_t n = dns_hijack(kQueryA, sizeof(kQueryA), kApAddr, reply, sizeof(reply));

    zassert_equal(n, sizeof(kQueryA) + kDnsAnswerBytes);

    /* The id comes back so the resolver can match it. */
    zassert_equal(reply[0], 0x12);
    zassert_equal(reply[1], 0x34);

    /* QR and AA set, RD echoed. */
    zassert_equal(reply[2] & 0x80, 0x80, "the reply must say it is a response");
    zassert_equal(reply[2] & 0x04, 0x04, "and that it is authoritative");
    zassert_equal(reply[2] & 0x01, 0x01, "RD is echoed back");
    zassert_equal(reply[3], 0x00, "no recursion available, and NOERROR");

    /* One question, one answer, nothing else. */
    zassert_equal(reply[5], 1);
    zassert_equal(reply[7], 1);
    zassert_equal(reply[9], 0);
    zassert_equal(reply[11], 0);

    const uint8_t *answer = &reply[sizeof(kQueryA)];

    /* The name is a pointer back to the question rather than a copy. */
    zassert_equal(answer[0], 0xC0);
    zassert_equal(answer[1], 12);
    zassert_equal(answer[3], kDnsTypeA);
    zassert_equal(answer[5], kDnsClassIn);

    /* TTL zero: the answer is only true on this network. */
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

ZTEST(tk_portal, test_dns_answers_aaaa_empty_rather_than_not_at_all)
{
    memcpy(query_aaaa, kQueryA, sizeof(kQueryA));
    query_aaaa[sizeof(kQueryA) - 3] = kDnsTypeAaaa;

    const uint16_t n = dns_hijack(query_aaaa, sizeof(query_aaaa), kApAddr, reply, sizeof(reply));

    zassert_equal(n, sizeof(kQueryA), "the question comes back with no answer after it");
    zassert_equal(reply[3], 0x00, "and NOERROR, so the client stops asking");
    zassert_equal(reply[7], 0, "ANCOUNT is zero");
}

ZTEST(tk_portal, test_dns_rejects_what_it_should_not_answer)
{
    DnsQuery q = {};
    uint8_t bad[sizeof(kQueryA)];

    zassert_false(dns_parse_query(kQueryA, 11, q), "shorter than a header");
    zassert_false(dns_parse_query(nullptr, sizeof(kQueryA), q));

    /* A response, not a query. */
    memcpy(bad, kQueryA, sizeof(bad));
    bad[2] |= 0x80;
    zassert_false(dns_parse_query(bad, sizeof(bad), q));

    /* An opcode other than QUERY. */
    memcpy(bad, kQueryA, sizeof(bad));
    bad[2] |= 0x08;
    zassert_false(dns_parse_query(bad, sizeof(bad), q));

    /* Two questions, which one answer would not cover. */
    memcpy(bad, kQueryA, sizeof(bad));
    bad[5] = 2;
    zassert_false(dns_parse_query(bad, sizeof(bad), q));

    /* A compression pointer, which has nothing to point at in a query. */
    memcpy(bad, kQueryA, sizeof(bad));
    bad[12] = 0xC0;
    zassert_false(dns_parse_query(bad, sizeof(bad), q));

    /* A label running past the end of the buffer — the case that would read
     * off the end of the packet if the length were trusted. */
    memcpy(bad, kQueryA, sizeof(bad));
    bad[12] = 0x3F;
    zassert_false(dns_parse_query(bad, sizeof(bad), q));

    /* Truncated before QTYPE and QCLASS. */
    zassert_false(dns_parse_query(kQueryA, sizeof(kQueryA) - 2, q));
}

ZTEST(tk_portal, test_dns_will_not_overrun_the_reply_buffer)
{
    DnsQuery q = {};

    zassert_true(dns_parse_query(kQueryA, sizeof(kQueryA), q));

    /* Exactly one byte short of what the answer needs. */
    const uint16_t n = dns_build_reply(kQueryA, sizeof(kQueryA), q, kApAddr, reply,
                                       sizeof(kQueryA) + kDnsAnswerBytes - 1);

    zassert_equal(n, 0);
}

/* ------------------------------------------------------------------ form */

ZTEST(tk_portal, test_form_reads_the_fields_a_browser_posts)
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

ZTEST(tk_portal, test_form_decodes_a_password_worth_escaping)
{
    /* Every character a form encodes, in one passphrase. */
    const char body[] = "psk=a%26b%3Dc%25d+e%2Bf";
    char value[kPskBufSize];

    zassert_equal(form_field(body, sizeof(body) - 1, "psk", value, sizeof(value)), 11);
    zassert_str_equal(value, "a&b=c%d e+f");
}

ZTEST(tk_portal, test_form_matches_whole_keys_only)
{
    const char body[] = "xssid=wrong&ssid_hidden=wrong&ssid=right";
    char value[kSsidBufSize];

    zassert_equal(form_field(body, sizeof(body) - 1, "ssid", value, sizeof(value)), 5);
    zassert_str_equal(value, "right");
}

ZTEST(tk_portal, test_form_rejects_rather_than_guesses)
{
    char value[kPskBufSize];

    /* A truncated escape. Passing the `%` through as a literal would save a
     * password that differs from the one somebody typed. */
    zassert_equal(form_field("psk=ab%", 7, "psk", value, sizeof(value)), -1);
    zassert_equal(form_field("psk=ab%4", 8, "psk", value, sizeof(value)), -1);

    /* A non-hex escape. */
    zassert_equal(form_field("psk=ab%zz", 9, "psk", value, sizeof(value)), -1);

    /* A key that is not there, and a field with no value at all. */
    zassert_equal(form_field("ssid=x", 6, "psk", value, sizeof(value)), -1);
    zassert_equal(form_field("psk", 3, "psk", value, sizeof(value)), -1);

    /* A value longer than the buffer, which is what a hostile body looks like. */
    char small[4];
    zassert_equal(form_field("psk=abcdef", 10, "psk", small, sizeof(small)), -1);
}

ZTEST(tk_portal, test_form_accepts_an_empty_value)
{
    char value[kPskBufSize];

    /* An open network has no password, and the field posts empty. */
    zassert_equal(form_field("ssid=x&psk=&lang=en", 19, "psk", value, sizeof(value)), 0);
    zassert_str_equal(value, "");
}

/* ------------------------------------------------------------------ page */

ZTEST(tk_portal, test_page_escapes_a_network_name_from_the_air)
{
    ScanEntry nets[1] = {};

    /* An SSID is 32 arbitrary bytes chosen by whoever runs that access point.
     * Printed raw into the page, this one would run. */
    strcpy(nets[0].ssid, "<script>alert(1)</script>");
    nets[0].secure = true;

    const int n = page_setup(page, sizeof(page), nets, 1, "en");

    zassert_true(n > 0, "the page has to fit");
    zassert_false(contains(page, "<script>"), "the tag must not survive into the page");
    zassert_true(contains(page, "&lt;script&gt;alert(1)&lt;/script&gt;"));
}

ZTEST(tk_portal, test_page_escapes_a_name_that_would_break_out_of_an_attribute)
{
    ScanEntry nets[1] = {};

    /* The SSID is also written into value="…", so a quote is the other way out. */
    strcpy(nets[0].ssid, "a\" onfocus=\"x");
    nets[0].secure = true;

    zassert_true(page_setup(page, sizeof(page), nets, 1, "en") > 0);
    zassert_false(contains(page, "onfocus=\"x\""));
    zassert_true(contains(page, "&quot; onfocus=&quot;x"));
}

ZTEST(tk_portal, test_page_setup_lists_networks_and_marks_the_language)
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
}

ZTEST(tk_portal, test_page_setup_still_takes_a_name_when_the_scan_found_nothing)
{
    zassert_true(page_setup(page, sizeof(page), nullptr, 0, "en") > 0);

    /* A hidden network has to be typeable, so the field is there either way. */
    zassert_true(contains(page, "name=\"ssid\""));
    zassert_true(contains(page, "No networks in range"));
}

ZTEST(tk_portal, test_page_status_never_shows_the_saved_password)
{
    const PortalStatus status = {
        .ap_ssid = "Tischkarte-A1B2",
        .board = "esp32s3_devkitc",
        .corpus_language = "en",
        .corpus_version = "2026.07.2",
        .corpus_count = 240,
        .saved_ssid = "Cafe Krone",
        .station_connected = true,
        .station_ip = "192.168.1.44",
    };

    zassert_true(page_status(page, sizeof(page), status) > 0);

    zassert_true(contains(page, "Tischkarte-A1B2"));
    zassert_true(contains(page, "esp32s3_devkitc"));
    zassert_true(contains(page, "240 in en"));
    zassert_true(contains(page, "2026.07.2"));
    zassert_true(contains(page, "Cafe Krone"));
    zassert_true(contains(page, "connected"));
    zassert_true(contains(page, "192.168.1.44"));
}

ZTEST(tk_portal, test_page_status_reports_an_unconfigured_device)
{
    const PortalStatus status = {
        .ap_ssid = "Tischkarte-A1B2",
        .board = "esp32s3_devkitc",
        .corpus_language = "en",
        .corpus_version = "2026.07.2",
        .corpus_count = 240,
        .saved_ssid = nullptr,
        .station_connected = false,
        .station_ip = "",
    };

    zassert_true(page_status(page, sizeof(page), status) > 0);
    zassert_true(contains(page, "none"), "no saved network says so plainly");
    zassert_true(contains(page, "not connected"));
}

ZTEST(tk_portal, test_page_reports_a_buffer_too_small_rather_than_writing_past_it)
{
    ScanEntry nets[1] = {};
    char tiny[64];

    strcpy(nets[0].ssid, "Krone");

    zassert_equal(page_setup(tiny, sizeof(tiny), nets, 1, "en"), -1);
    zassert_equal(page_saved(tiny, sizeof(tiny), "Krone"), -1);
}

ZTEST(tk_portal, test_page_saved_names_the_network_being_joined)
{
    zassert_true(page_saved(page, sizeof(page), "Cafe & Bar") > 0);
    zassert_true(contains(page, "Cafe &amp; Bar"));
}

ZTEST(tk_portal, test_html_escape_covers_every_character_that_matters)
{
    char out[64];
    const char in[] = "&<>\"'";

    zassert_equal(html_escape(in, sizeof(in) - 1, out, sizeof(out)), 24);
    zassert_str_equal(out, "&amp;&lt;&gt;&quot;&#39;");
}

ZTEST(tk_portal, test_html_escape_reports_a_buffer_too_small)
{
    char out[8];

    zassert_equal(html_escape("&&&&", 4, out, sizeof(out)), -1);
}

/* ------------------------------------------------------------------- FSM */

namespace
{

/* STATE() casts through this name, so the macro needs it in scope. */
using State = PortalFsm::State;

/** Records what the state machine asked for, and answers how the test says to. */
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

/** Same machine, with a clock the test winds forward by hand. */
class TestPortalFsm : public PortalFsm
{
public:
    using PortalFsm::PortalFsm;

    int64_t clock = 0;

    void advance(int64_t ms) { clock += ms; }

protected:
    int64_t now_ms() const override { return clock; }
};

/** Tick until the state stops moving, as the net thread's loop does. */
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

/** Drive a fresh machine as far as SERVING, which most cases start from. */
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

ZTEST(tk_portal, test_fsm_starts_off_and_stays_there)
{
    FakeIo io;
    TestPortalFsm fsm(io);

    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(OFF));
    zassert_false(fsm.is_active(), "nothing is on air until somebody asks");
    zassert_equal(io.scans, 0);
    zassert_equal(io.ap_starts, 0);
}

ZTEST(tk_portal, test_fsm_scans_before_it_raises_the_access_point)
{
    FakeIo io;
    TestPortalFsm fsm(io);

    fsm.post_start();
    settle(fsm);

    /* One radio: the scan has to finish before the AP takes it. */
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

ZTEST(tk_portal, test_fsm_gives_up_on_a_scan_that_never_reports)
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

    /* Onward, not away: the page still takes a typed name, which a hidden
     * network needs anyway. */
    zassert_equal(fsm.get_current_state(), STATE(AP_STARTING));
}

ZTEST(tk_portal, test_fsm_carries_on_when_the_scan_cannot_be_started)
{
    FakeIo io;
    TestPortalFsm fsm(io);

    io.scan_succeeds = false;

    fsm.post_start();
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(AP_STARTING),
                  "no waiting for a scan that is not running");
}

ZTEST(tk_portal, test_fsm_ends_when_the_access_point_will_not_come_up)
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

ZTEST(tk_portal, test_fsm_ends_when_nothing_can_be_served)
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

    /* An access point that answers no request is worse than none: it would sit
     * there for the whole window looking like it worked. */
    zassert_equal(fsm.get_current_state(), STATE(OFF));
    zassert_equal(io.teardowns, 1);
}

ZTEST(tk_portal, test_fsm_joins_a_network_when_the_form_arrives)
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

ZTEST(tk_portal, test_fsm_puts_a_refused_password_back_on_the_form)
{
    FakeIo io;
    TestPortalFsm fsm(io);

    reach_serving(fsm, io);

    fsm.post_credentials();
    settle(fsm);

    fsm.post_connected(false);
    settle(fsm);

    /* The likely cause is a mistyped password, and the fix is to type it again
     * rather than to start the whole gesture over. */
    zassert_equal(fsm.get_current_state(), STATE(SERVING));
    zassert_equal(io.last_card, PortalCard::REFUSED);
    zassert_equal(io.serve_starts, 1, "the access point never went down, so nothing restarts");
    zassert_equal(io.ap_starts, 1);

    /* And a second attempt still works. */
    fsm.post_credentials();
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(CONNECTING));
    zassert_equal(io.connects, 2);
}

ZTEST(tk_portal, test_fsm_treats_a_silent_network_as_a_refusal)
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

ZTEST(tk_portal, test_fsm_closes_the_window_on_its_own)
{
    FakeIo io;
    TestPortalFsm fsm(io);

    reach_serving(fsm, io);

    fsm.advance(kPortalWindowMs - 1);
    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(SERVING));

    fsm.advance(1);
    settle(fsm);

    /* An open access point nobody is using should not stay on a table. */
    zassert_equal(fsm.get_current_state(), STATE(OFF));
    zassert_equal(io.teardowns, 1);
    zassert_false(fsm.is_active());
}

ZTEST(tk_portal, test_fsm_gives_the_status_page_a_window_of_its_own)
{
    FakeIo io;
    TestPortalFsm fsm(io);

    reach_serving(fsm, io);

    /* Spend most of the first window on the form, then join. */
    fsm.advance(kPortalWindowMs - 10);
    settle(fsm);

    fsm.post_credentials();
    settle(fsm);
    fsm.post_connected(true);
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(CONNECTED));

    /* The clock started again on the way in, so the page is readable. */
    fsm.advance(kPortalWindowMs - 1);
    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(CONNECTED));

    fsm.advance(1);
    settle(fsm);
    zassert_equal(fsm.get_current_state(), STATE(OFF));
    zassert_equal(io.teardowns, 1);
}

ZTEST(tk_portal, test_fsm_stops_from_wherever_it_is)
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

ZTEST(tk_portal, test_fsm_ignores_a_stop_that_arrives_with_nothing_running)
{
    FakeIo io;
    TestPortalFsm fsm(io);

    fsm.post_stop();
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(OFF));
    zassert_equal(io.teardowns, 0);

    /* And it must not be left queued to end the next session immediately. */
    fsm.post_start();
    settle(fsm);

    zassert_equal(fsm.get_current_state(), STATE(SCANNING));
}
