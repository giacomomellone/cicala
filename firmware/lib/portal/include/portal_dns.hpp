/*
 * The DNS half of a captive portal: answer every name with our own address.
 *
 * No Zephyr headers, so the suite runs real query bytes through it on the host.
 *
 * ## Why this is written rather than configured
 *
 * A phone decides it is behind a captive portal by asking for a known name and
 * seeing the wrong answer. That needs a DNS server on the setup network which
 * resolves everything to the device, and Zephyr has no such thing: the DNS
 * library ships a resolver, an mDNS responder and an LLMNR responder, none of
 * which answers arbitrary names on behalf of somebody else.
 *
 * What is needed is small — read a query, write one A record — so it lives here
 * as bytes in and bytes out, and app/src/portal.c only owns the socket.
 *
 * ## What is answered, and what is not
 *
 * An A query gets the address. Anything else — AAAA above all, which every
 * phone asks for alongside the A — gets an empty NOERROR reply rather than
 * silence, because a client that gets no answer at all retries and waits
 * instead of falling back to IPv4.
 *
 * Malformed queries get nothing. This is the side reading packets off a network
 * anyone can join, so it is strict in the same way qdb::open() is strict: every
 * declared length is checked against the buffer before it is trusted.
 */

#pragma once

#include <stdint.h>

namespace tk
{

/** Classic DNS over UDP, before EDNS0 raises it. Bounds every buffer here. */
constexpr uint16_t kDnsMaxMessage = 512;

/** The longest legal encoded name: 255 bytes including the length octets. */
constexpr uint16_t kDnsMaxName = 255;

/** Bytes a reply adds to the query it echoes: 12 pointer + type + class + TTL + rdata. */
constexpr uint16_t kDnsAnswerBytes = 16;

constexpr uint16_t kDnsTypeA = 1;
constexpr uint16_t kDnsTypeAaaa = 28;
constexpr uint16_t kDnsClassIn = 1;

/** What was asked, once the question section has been checked. */
struct DnsQuery {
    uint16_t id;
    uint16_t qtype;
    uint16_t qclass;
    /** Where the encoded name starts in the query, and how long it is. */
    uint16_t name_offset;
    uint16_t name_len;
    /** First byte after the question section. */
    uint16_t question_end;
    /** The client asked us to recurse; echoed back so it is not confused. */
    bool recursion_desired;
};

/**
 * Read the question out of a query.
 *
 * Rejects anything that is not a single-question standard query: responses,
 * opcodes other than QUERY, a question count that is not one, a compression
 * pointer (illegal in the only name a query carries), an over-long label or
 * name, and any length that runs past the end of the buffer.
 *
 * @return false when the query is not one worth answering.
 */
bool dns_parse_query(const uint8_t *msg, uint16_t len, DnsQuery &out);

/**
 * Build the reply to a parsed query.
 *
 * An A question in class IN is answered with `addr`; every other type gets the
 * same reply with no answer record. The name is echoed by pointing at the one
 * already in the question, which keeps the reply inside the size of the query
 * plus kDnsAnswerBytes.
 *
 * @param addr  the address to hand out, in host byte order
 * @return bytes written, or 0 when `out` is too small.
 */
uint16_t dns_build_reply(const uint8_t *msg, uint16_t len, const DnsQuery &query, uint32_t addr,
                         uint8_t *out, uint16_t out_size);

/**
 * Parse and answer in one call, which is all the socket loop wants.
 *
 * @return bytes written to `out`, or 0 when nothing should be sent.
 */
uint16_t dns_hijack(const uint8_t *msg, uint16_t len, uint32_t addr, uint8_t *out,
                    uint16_t out_size);

} // namespace tk
