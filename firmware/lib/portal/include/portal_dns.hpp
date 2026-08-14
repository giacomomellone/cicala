/* Captive DNS parser and responder. A queries resolve to the device address. */

#pragma once

#include <stdint.h>

namespace tk
{

/** Maximum classic DNS UDP message size. */
constexpr uint16_t kDnsMaxMessage = 512;

/** The longest legal encoded name: 255 bytes including the length octets. */
constexpr uint16_t kDnsMaxName = 255;

/** Bytes added by a compressed A-record answer. */
constexpr uint16_t kDnsAnswerBytes = 16;

constexpr uint16_t kDnsTypeA = 1;
constexpr uint16_t kDnsTypeAaaa = 28;
constexpr uint16_t kDnsClassIn = 1;

/** Validated DNS question fields. */
struct DnsQuery {
    uint16_t id;
    uint16_t qtype;
    uint16_t qclass;
    /** Encoded query-name location. */
    uint16_t name_offset;
    uint16_t name_len;
    /** First byte after the question section. */
    uint16_t question_end;
    /** Client recursion flag, echoed in the response. */
    bool recursion_desired;
};

/** Parse one standard uncompressed question. */
bool dns_parse_query(const uint8_t *msg, uint16_t len, DnsQuery &out);

/** Build a captive reply. `addr` is in host byte order. */
uint16_t dns_build_reply(const uint8_t *msg, uint16_t len, const DnsQuery &query, uint32_t addr,
                         uint8_t *out, uint16_t out_size);

/** Parse and answer. Return zero for invalid input or a small output buffer. */
uint16_t dns_hijack(const uint8_t *msg, uint16_t len, uint32_t addr, uint8_t *out,
                    uint16_t out_size);

} // namespace tk
