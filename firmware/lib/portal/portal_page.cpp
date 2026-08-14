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
    "*{box-sizing:border-box}body{font:16px/1.5 system-ui,sans-serif;margin:0;"
    "padding:1.25rem;background:#eeeae2;color:#272522}main{max-width:34rem;margin:0 auto}"
    ".brand{display:flex;gap:.75rem;align-items:center;margin:.25rem 0 1.25rem}"
    ".mark{display:grid;place-items:center;width:2.5rem;height:2.5rem;border-radius:.75rem;"
    "background:#272522;color:#f8f5ef;font-weight:700;letter-spacing:-.08em}"
    ".eyebrow{font-size:.7rem;letter-spacing:.12em;font-weight:700;color:#81786d}"
    "h1{font-size:1.35rem;line-height:1.2;margin:.1rem 0 0;letter-spacing:-.02em}"
    "h2{font-size:1rem;margin:0;font-weight:700}label{display:block;margin:1rem 0 .3rem;"
    "font-weight:650}"
    "select,input{width:100%;padding:.6rem;font-size:1rem;box-sizing:border-box;"
    "border:1px solid #c8c0b5;border-radius:.5rem;background:#fff;color:inherit}"
    "button{margin-top:1rem;width:100%;padding:.7rem;font-size:1rem;font-weight:650;"
    "border:0;border-radius:.5rem;background:#272522;color:#fff;cursor:pointer}"
    "button.quiet{background:#e2ddd4;color:#3d3934}button.danger{background:#fff;"
    "border:1px solid #c9a59a;color:#8b3f32}"
    ".panel{background:#fdfcf9;border:1px solid #d9d2c8;border-radius:.75rem;"
    "padding:1rem;margin:0 0 .85rem;box-shadow:0 1px 1px #d9d2c833}"
    ".panel-head{display:flex;justify-content:space-between;align-items:baseline;gap:1rem}"
    ".lede{color:#6f675e;margin:-.5rem 0 1.1rem}.note{color:#81786d;font-size:.9rem}"
    ".rule{border:0;border-top:1px solid #e4ded5;margin:1rem 0}.actions{display:flex;gap:.6rem;"
    "flex-wrap:wrap}.actions form{flex:1;min-width:10rem}.actions button{margin-top:0}"
    "dl{display:grid;grid-template-columns:auto 1fr;gap:.45rem 1rem;margin:1rem 0 0}"
    "dt{font-weight:650;color:#625b53}dd{margin:0;text-align:right;overflow-wrap:anywhere}"
    ".pill{display:inline-block;border-radius:99px;padding:.15rem .55rem;font-size:.8rem;"
    "font-weight:700;background:#e8e2d8}.pill.good{background:#dfe9dc;color:#42613d}"
    ".pill.warn{background:#f1ddd4;color:#8b3f32}a{color:#514a43}"
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
    w.raw("<header class=\"brand\"><span class=\"mark\">tk</span><div>"
          "<div class=\"eyebrow\">TISCHKARTE DEVICE</div><h1>Set up your table</h1>"
          "</div></header><p class=\"lede\">Give the device a network and choose the language "
          "for its questions.</p>");
    w.raw("<section class=\"panel\"><div class=\"panel-head\"><h2>Wi-Fi</h2>"
          "<span class=\"note\">saved on the device</span></div>");
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
    w.raw("<p class=\"note\">Leave empty for an open network.</p>");
    w.raw("<button type=\"submit\">Save network</button>");
    w.raw("</form>");

    w.raw("<form method=\"post\" action=\"/scan\"><button class=\"quiet\" type=\"submit\">"
          "Scan again</button></form></section>");

    w.raw("<section class=\"panel\"><div class=\"panel-head\"><h2>Questions</h2>"
          "<span class=\"note\">stored offline</span></div>");
    w.raw("<form method=\"post\" action=\"/language\">");
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

    w.raw("</select><button type=\"submit\">Save language</button></form></section>");

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
    w.raw("<header class=\"brand\"><span class=\"mark\">tk</span><div>"
          "<div class=\"eyebrow\">TISCHKARTE DEVICE</div><h1>Saved</h1></div></header>");

    if (ssid == nullptr || ssid[0] == '\0') {
        w.raw("<p class=\"lede\">The language is set. It applies to the next question.</p>");
        nav(w, false);
        w.raw(kFoot);

        return w.finish();
    }

    w.raw("<p class=\"lede\">Joining ");
    w.text(ssid);
    w.raw(".</p>");

    // The panel reports the result after the phone leaves the access point.
    w.raw("<p class=\"note\">The setup network stays up for a few minutes. "
          "Check the status page to see whether it worked.</p>");

    nav(w, false);
    w.raw(kFoot);

    return w.finish();
}

int page_notice(char *out, uint16_t out_size, const char *heading, const char *body)
{
    if (out == nullptr) {
        return -1;
    }

    Writer w(out, out_size);

    w.raw(kHead);
    w.raw("<header class=\"brand\"><span class=\"mark\">tk</span><div>"
          "<div class=\"eyebrow\">TISCHKARTE DEVICE</div><h1>");
    w.text(heading != nullptr ? heading : "Tischkarte");
    w.raw("</h1></div></header><p class=\"lede\">");
    w.text(body != nullptr ? body : "Done.");
    w.raw("</p>");
    nav(w, false);
    w.raw(kFoot);

    return w.finish();
}

int page_forget_confirm(char *out, uint16_t out_size)
{
    if (out == nullptr) {
        return -1;
    }

    Writer w(out, out_size);

    w.raw(kHead);
    w.raw("<header class=\"brand\"><span class=\"mark\">tk</span><div>"
          "<div class=\"eyebrow\">TISCHKARTE DEVICE</div><h1>Forget saved Wi-Fi?</h1>"
          "</div></header><p class=\"lede\">This removes every saved network from the device. "
          "Your questions and language stay intact.</p>");
    w.raw("<form method=\"post\" action=\"/forget\"><input type=\"hidden\" name=\"confirm\" "
          "value=\"forget\"><button class=\"danger\" type=\"submit\">Confirm and forget Wi-Fi</button></form>");
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
    w.raw("<header class=\"brand\"><span class=\"mark\">tk</span><div>"
          "<div class=\"eyebrow\">TISCHKARTE DEVICE</div><h1>Device status</h1>"
          "</div></header><section class=\"panel\"><div class=\"panel-head\"><h2>Connection</h2>");

    if (status.station_connected) {
        w.raw("<span class=\"pill good\">Connected</span>");
    } else {
        w.raw("<span class=\"pill warn\">Offline</span>");
    }

    w.raw("</div><dl>");
    w.raw("<dt>Network</dt><dd>");
    if (status.saved_ssid != nullptr && status.saved_ssid[0] != '\0') {
        w.text(status.saved_ssid);
    } else {
        w.raw("none saved");
    }
    w.raw("</dd><dt>Address</dt><dd>");
    if (status.station_connected && status.station_ip != nullptr && status.station_ip[0] != '\0') {
        w.text(status.station_ip);
    } else {
        w.raw("not connected");
    }
    w.raw("</dd><dt>Portal</dt><dd>");
    if (status.window_remaining_s > 0) {
        w.number(status.window_remaining_s);
        w.raw(" seconds left");
    } else {
        w.raw("closing soon");
    }
    w.raw("</dd></dl>");

    if (status.connection_error != nullptr && status.connection_error[0] != '\0') {
        w.raw("<p class=\"note\">");
        w.text(status.connection_error);
        w.raw(". Check the password and try again.</p>");
    }
    w.raw("</section><section class=\"panel\"><div class=\"panel-head\"><h2>Device data</h2>"
          "<span class=\"note\">available offline</span></div><dl>");

    w.raw("<dt>Setup network</dt><dd>");
    w.text(status.ap_ssid != nullptr ? status.ap_ssid : "");
    w.raw("</dd><dt>Board</dt><dd>");
    w.text(status.board != nullptr ? status.board : "unknown");
    w.raw("</dd><dt>Firmware</dt><dd>");
    w.text(status.firmware_version != nullptr ? status.firmware_version : "unknown");
    w.raw("</dd><dt>Questions</dt><dd>");
    w.number(status.corpus_count);
    w.raw(" in ");
    w.text(status.corpus_language != nullptr ? status.corpus_language : "?");
    w.raw("</dd><dt>Corpus</dt><dd>");
    w.text(status.corpus_version != nullptr && status.corpus_version[0] != '\0'
               ? status.corpus_version
               : "built in");
    w.raw("</dd></dl></section>");

    w.raw("<section class=\"panel\"><h2>Maintenance</h2><div class=\"actions\">");
    // POST prevents link prefetching from starting a sync.
    w.raw("<form method=\"post\" action=\"/sync\"><button type=\"submit\">Check for updates</button></form>");
    w.raw("<form method=\"post\" action=\"/forget\"><button class=\"danger\" type=\"submit\">"
          "Forget saved Wi-Fi</button></form></div><p class=\"note\">");
    if (status.sync_result != nullptr && status.sync_result[0] != '\0') {
        w.text(status.sync_result);
    } else {
        w.raw("The update result will appear on the device and here after you reload this page.");
    }
    w.raw("</p></section>");

    nav(w, true);
    w.raw(kFoot);

    return w.finish();
}

} // namespace tk
