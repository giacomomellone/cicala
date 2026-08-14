#include "portal_page.hpp"

namespace tk
{

namespace
{

/** Languages available in the compiled corpus. */
struct Language {
    const char *code;
    const char *name;
};

constexpr Language kLanguages[] = {
    {"en", "English"},
    {"de", "Deutsch"},
};

/** Bounds-checked append-only writer over a fixed buffer. */
class Writer
{
public:
    Writer(char *out, uint16_t size) : _out(out), _size(size) {}

    void raw(const char *s)
    {
        while (*s != '\0') {
            put(*s);
            s++;
        }
    }

    /** Append HTML-escaped text. */
    void text(const char *s)
    {
        while (*s != '\0') {
            switch (*s) {
            case '&':
                raw("&amp;");
                break;
            case '<':
                raw("&lt;");
                break;
            case '>':
                raw("&gt;");
                break;
            case '"':
                raw("&quot;");
                break;
            case '\'':
                raw("&#39;");
                break;
            default:
                put(*s);
                break;
            }

            s++;
        }
    }

    void number(uint32_t v)
    {
        char digits[10];
        uint8_t n = 0;

        do {
            digits[n] = (char) ('0' + (v % 10));
            n++;
            v /= 10;
        } while (v != 0 && n < sizeof(digits));

        while (n > 0) {
            n--;
            put(digits[n]);
        }
    }

    /** Return bytes written, or -1 after an overflow. */
    int finish()
    {
        if (_overflowed || _at >= _size) {
            return -1;
        }

        _out[_at] = '\0';

        return (int) _at;
    }

private:
    void put(char c)
    {
        // Reserve one byte for the terminator.
        if (_at + 1 >= _size) {
            _overflowed = true;
            return;
        }

        _out[_at] = c;
        _at++;
    }

    char *_out;
    uint16_t _size;
    uint16_t _at = 0;
    bool _overflowed = false;
};

// Captive clients cannot fetch an external stylesheet.
const char *const kHead =
    "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\">"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<title>Tischkarte setup</title><style>"
    "body{font:16px/1.5 system-ui,sans-serif;margin:0;padding:1.5rem;"
    "background:#f7f5f1;color:#2b2b2b}"
    "main{max-width:26rem;margin:0 auto}"
    "h1{font-size:1.25rem;margin:0 0 1rem}"
    "label{display:block;margin:1rem 0 .25rem;font-weight:600}"
    "select,input{width:100%;padding:.6rem;font-size:1rem;box-sizing:border-box;"
    "border:1px solid #b3aca0;border-radius:.25rem;background:#fff}"
    "button{margin-top:1.25rem;width:100%;padding:.75rem;font-size:1rem;"
    "border:0;border-radius:.25rem;background:#2b2b2b;color:#fff}"
    "dl{display:grid;grid-template-columns:auto 1fr;gap:.25rem 1rem;margin:0}"
    "dt{font-weight:600}dd{margin:0}"
    "p.note{color:#7a736a}"
    "a{color:#2b2b2b}"
    "</style></head><body><main>";

const char *const kFoot = "</main></body></html>";

void nav(Writer &w, bool on_status)
{
    w.raw("<p class=\"note\">");

    if (on_status) {
        w.raw("<a href=\"/\">Back to setup</a>");
    } else {
        w.raw("<a href=\"/status\">Device status</a>");
    }

    w.raw("</p>");
}

} // namespace

int html_escape(const char *in, uint16_t len, char *out, uint16_t out_size)
{
    if (in == nullptr || out == nullptr || out_size == 0) {
        return -1;
    }

    Writer w(out, out_size);

    for (uint16_t i = 0; i < len; i++) {
        const char one[2] = {in[i], '\0'};

        w.text(one);
    }

    return w.finish();
}

int page_setup(char *out, uint16_t out_size, const ScanEntry *nets, uint8_t count,
               const char *current_language)
{
    if (out == nullptr) {
        return -1;
    }

    Writer w(out, out_size);

    w.raw(kHead);
    w.raw("<h1>Tischkarte setup</h1>");
    w.raw("<form method=\"post\" action=\"/save\">");

    w.raw("<label for=\"ssid\">Network</label>");

    if (count == 0 || nets == nullptr) {
        // Keep hidden networks usable when the scan is empty.
        w.raw("<input id=\"ssid\" name=\"ssid\" placeholder=\"Network name\">");
        w.raw("<p class=\"note\">No networks in range. Type the name.</p>");
    } else {
        w.raw("<select id=\"ssid\" name=\"ssid\">");

        w.raw("<option value=\"\">Leave unchanged</option>");

        for (uint8_t i = 0; i < count; i++) {
            w.raw("<option value=\"");
            w.text(nets[i].ssid);
            w.raw("\">");
            w.text(nets[i].ssid);

            if (!nets[i].secure) {
                w.raw(" (open)");
            }

            w.raw("</option>");
        }

        w.raw("</select>");
    }

    w.raw("<label for=\"psk\">Password</label>");
    w.raw("<input id=\"psk\" name=\"psk\" type=\"password\" "
          "autocomplete=\"off\" autocapitalize=\"off\">");
    w.raw("<p class=\"note\">Leave empty for an open network. To change only the "
          "language, clear the network name and save.</p>");

    w.raw("<label for=\"lang\">Question language</label>");
    w.raw("<select id=\"lang\" name=\"lang\">");

    for (const Language &lang : kLanguages) {
        w.raw("<option value=\"");
        w.text(lang.code);
        w.raw("\"");

        if (current_language != nullptr && current_language[0] == lang.code[0] &&
            current_language[1] == lang.code[1]) {
            w.raw(" selected");
        }

        w.raw(">");
        w.text(lang.name);
        w.raw("</option>");
    }

    w.raw("</select>");
    w.raw("<button type=\"submit\">Save</button>");
    w.raw("</form>");

    nav(w, false);
    w.raw(kFoot);

    return w.finish();
}

int page_saved(char *out, uint16_t out_size, const char *ssid)
{
    if (out == nullptr) {
        return -1;
    }

    Writer w(out, out_size);

    w.raw(kHead);
    w.raw("<h1>Saved</h1>");

    if (ssid == nullptr || ssid[0] == '\0') {
        w.raw("<p>The language is set. It applies to the next question.</p>");
        nav(w, false);
        w.raw(kFoot);

        return w.finish();
    }

    w.raw("<p>Joining ");
    w.text(ssid);
    w.raw(".</p>");

    // The panel reports the result after the phone leaves the access point.
    w.raw("<p class=\"note\">The setup network stays up for a few minutes. "
          "Check the status page to see whether it worked.</p>");

    nav(w, false);
    w.raw(kFoot);

    return w.finish();
}

int page_status(char *out, uint16_t out_size, const PortalStatus &status)
{
    if (out == nullptr) {
        return -1;
    }

    Writer w(out, out_size);

    w.raw(kHead);
    w.raw("<h1>Device status</h1><dl>");

    w.raw("<dt>Board</dt><dd>");
    w.text(status.board != nullptr ? status.board : "unknown");
    w.raw("</dd>");

    w.raw("<dt>Setup network</dt><dd>");
    w.text(status.ap_ssid != nullptr ? status.ap_ssid : "");
    w.raw("</dd>");

    w.raw("<dt>Questions</dt><dd>");
    w.number(status.corpus_count);
    w.raw(" in ");
    w.text(status.corpus_language != nullptr ? status.corpus_language : "?");
    w.raw("</dd>");

    w.raw("<dt>Corpus</dt><dd>");
    w.text(status.corpus_version != nullptr ? status.corpus_version : "unknown");
    w.raw("</dd>");

    // Never render the saved password on the open access point.
    w.raw("<dt>Saved network</dt><dd>");

    if (status.saved_ssid != nullptr && status.saved_ssid[0] != '\0') {
        w.text(status.saved_ssid);
    } else {
        w.raw("none");
    }

    w.raw("</dd>");

    w.raw("<dt>Connection</dt><dd>");

    if (status.station_connected) {
        w.raw("connected");

        if (status.station_ip != nullptr && status.station_ip[0] != '\0') {
            w.raw(", ");
            w.text(status.station_ip);
        }
    } else {
        w.raw("not connected");
    }

    w.raw("</dd></dl>");

    // POST prevents link prefetching from starting a sync.
    w.raw("<form method=\"post\" action=\"/sync\">");
    w.raw("<button type=\"submit\">Check for new questions</button>");
    w.raw("</form>");
    w.raw("<p class=\"note\">The result appears on the device, and stays there "
          "until the next press.</p>");

    nav(w, true);
    w.raw(kFoot);

    return w.finish();
}

} // namespace tk
