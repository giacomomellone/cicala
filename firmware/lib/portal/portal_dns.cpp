#include "portal_dns.hpp"

namespace tk
{

namespace
{

/** Fixed part of every DNS message: id, flags, and the four section counts. */
constexpr uint16_t kHeaderBytes = 12;

/** A label's length octet may not exceed this; 0xC0 and above is a pointer. */
constexpr uint8_t kMaxLabel = 63;
constexpr uint8_t kPointerMask = 0xC0;

/* Flag bits in the second and third header bytes. */
constexpr uint8_t kFlagQuery = 0x80; ///< high byte: QR, set on a response
constexpr uint8_t kFlagAuthoritative = 0x04;
constexpr uint8_t kFlagRecursionDesired = 0x01;
constexpr uint8_t kOpcodeMask = 0x78; ///< high byte: OPCODE, zero for a query

/**
 * Time to live, in seconds.
 *
 * Zero, and deliberately. The answer is a lie that is only true on this
 * network, so a phone that caches it would keep resolving the whole internet to
 * 192.168.4.1 after it walks away from the table.
 */
constexpr uint32_t kAnswerTtl = 0;

uint16_t read_u16(const uint8_t *p)
{
    return (uint16_t) ((uint16_t) p[0] << 8 | p[1]);
}

void write_u16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t) (v >> 8);
    p[1] = (uint8_t) (v & 0xFF);
}

} // namespace

bool dns_parse_query(const uint8_t *msg, uint16_t len, DnsQuery &out)
{
    if (msg == nullptr || len < kHeaderBytes || len > kDnsMaxMessage) {
        return false;
    }

    /* A response arriving on the server port is somebody else's business. */
    if ((msg[2] & kFlagQuery) != 0) {
        return false;
    }

    /* Only a standard query. Inverse queries and status requests are not what
     * a phone probing for a portal sends, and answering them as if they were
     * A lookups would be worse than staying quiet. */
    if ((msg[2] & kOpcodeMask) != 0) {
        return false;
    }

    /* Exactly one question. Multi-question queries are legal on paper and
     * unheard of in practice, and one answer would not cover them. */
    if (read_u16(&msg[4]) != 1) {
        return false;
    }

    const uint16_t name_offset = kHeaderBytes;
    uint16_t at = name_offset;

    /* Walk the labels. The name ends at a zero-length one. */
    while (true) {
        if (at >= len) {
            return false;
        }

        const uint8_t label = msg[at];

        /* A compression pointer needs a prior name to point at, and this is the
         * first name in the message. Nothing legal can appear here. */
        if ((label & kPointerMask) != 0) {
            return false;
        }

        if (label > kMaxLabel) {
            return false;
        }

        at++;

        if (label == 0) {
            break;
        }

        if ((uint32_t) at + label > len) {
            return false;
        }

        at = (uint16_t) (at + label);

        if (at - name_offset > kDnsMaxName) {
            return false;
        }
    }

    /* QTYPE and QCLASS follow the name. */
    if ((uint32_t) at + 4 > len) {
        return false;
    }

    out.id = read_u16(&msg[0]);
    out.recursion_desired = (msg[2] & kFlagRecursionDesired) != 0;
    out.name_offset = name_offset;
    out.name_len = (uint16_t) (at - name_offset);
    out.qtype = read_u16(&msg[at]);
    out.qclass = read_u16(&msg[at + 2]);
    out.question_end = (uint16_t) (at + 4);

    return true;
}

uint16_t dns_build_reply(const uint8_t *msg, uint16_t len, const DnsQuery &query, uint32_t addr,
                         uint8_t *out, uint16_t out_size)
{
    if (msg == nullptr || out == nullptr || query.question_end > len) {
        return 0;
    }

    /* Only an A record in class IN gets an address. Everything else is
     * answered empty, which is what stops a phone waiting on its AAAA. */
    const bool answered = query.qtype == kDnsTypeA && query.qclass == kDnsClassIn;
    const uint16_t needed = (uint16_t) (query.question_end + (answered ? kDnsAnswerBytes : 0));

    if (needed > out_size) {
        return 0;
    }

    /* Header and question come back unchanged apart from the flags. */
    for (uint16_t i = 0; i < query.question_end; i++) {
        out[i] = msg[i];
    }

    out[2] = kFlagQuery | kFlagAuthoritative;

    if (query.recursion_desired) {
        out[2] |= kFlagRecursionDesired;
    }

    /* Recursion is not available and the response code is NOERROR, which is
     * the whole of the low flag byte. */
    out[3] = 0;

    write_u16(&out[6], answered ? 1 : 0); /* ANCOUNT */
    write_u16(&out[8], 0);                /* NSCOUNT */
    write_u16(&out[10], 0);               /* ARCOUNT */

    if (!answered) {
        return query.question_end;
    }

    uint8_t *ans = &out[query.question_end];

    /* Point at the name already in the question rather than repeating it. */
    write_u16(&ans[0], (uint16_t) (kPointerMask << 8 | query.name_offset));
    write_u16(&ans[2], kDnsTypeA);
    write_u16(&ans[4], kDnsClassIn);
    write_u16(&ans[6], (uint16_t) (kAnswerTtl >> 16));
    write_u16(&ans[8], (uint16_t) (kAnswerTtl & 0xFFFF));
    write_u16(&ans[10], 4); /* RDLENGTH */

    ans[12] = (uint8_t) (addr >> 24);
    ans[13] = (uint8_t) (addr >> 16);
    ans[14] = (uint8_t) (addr >> 8);
    ans[15] = (uint8_t) (addr);

    return needed;
}

uint16_t dns_hijack(const uint8_t *msg, uint16_t len, uint32_t addr, uint8_t *out,
                    uint16_t out_size)
{
    DnsQuery query = {};

    if (!dns_parse_query(msg, len, query)) {
        return 0;
    }

    return dns_build_reply(msg, len, query, addr, out, out_size);
}

} // namespace tk
