/*
 * The decision maker, wired to the question store and the display channel.
 *
 * The corpus is compiled into the image. That is the factory-preloaded set
 * docs/design.md promises: a device whose Wi-Fi is never configured still
 * works. When `sync` lands it will mount a downloaded bundle from LittleFS
 * instead, and the only thing that changes here is where `open()` points.
 */

#include "app_logic.h"

#include <string.h>

#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

#include "app_fsm.hpp"
#include "channels.h"
#include "qdb.hpp"
#include "retained_block.hpp"

LOG_MODULE_REGISTER(tk_app, LOG_LEVEL_INF);

namespace
{

const uint8_t corpus[] = {
#include "corpus.inc"
};

/*
 * Bag state lives in RTC slow memory, so the no-repeat cycle survives the
 * reboot that deep sleep really is. CONFIG_PM is still off for the breadboard
 * stage, so nothing sleeps yet — but a reset button and a deep-sleep wake are
 * the same event as far as this is concerned, which is what makes it testable
 * before the sleep path exists.
 */
tk::Bag::State &bag_state = tk_retained().bag;

uint32_t random_u32(void *ctx)
{
    ARG_UNUSED(ctx);

    return sys_rand32_get();
}

#ifdef CONFIG_TK_DEBUG_CHARSET
/*
 * Character-set test pages, shown instead of questions when
 * CONFIG_TK_DEBUG_CHARSET is on. Next steps through them and wraps around.
 *
 * The point is to put every glyph the renderer can produce in front of a
 * camera: the ASCII the font actually contains, then each shipped or planned
 * language in its own idiom, then the diacritics on their own so a misplaced
 * mark is obvious rather than buried in a word. Each page is logged as it is
 * drawn, so the console says what the panel should be showing.
 */
const char *const debug_pages[] = {
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
    "abcdefghijklmnopqrstuvwxyz",
    "0123456789 .,;:!?'\"-()[]{}",
    "Grüße aus Köln: schöne Wörter für Käse und Straße.",
    "Ça va très bien où êtes-vous? Cœur naïf, août, déjà.",
    "¿Qué añoras? ¡Corazón! Más allá del jardín pequeño.",
    "Perché è così? Città, però, virtù, lunedì, più.",
    "áàâäã éèêë íìîï óòôöõ úùûü ýÿ ç ñ ß œ æ",
    "ÁÀÂÄÃ ÉÈÊË ÍÌÎÏ ÓÒÔÖÕ ÚÙÛÜ Ý Ç Ñ Œ Æ",
    "Which object within sight would be hardest to explain to someone "
    "from the past?",
};
#endif

/** Draws from the bag and hands the result to the display. */
class Io : public tk::AppIo
{
public:
    tk::Qdb qdb;
    tk::Bag bag{bag_state, random_u32, nullptr};

    bool draw(uint8_t deck) override
    {
        uint16_t index = 0;
        tk::Question question;

#ifdef CONFIG_TK_DEBUG_CHARSET
        const char *const page = debug_pages[_page];

        _page = (_page + 1) % ARRAY_SIZE(debug_pages);

        question.text = page;
        question.len = static_cast<uint16_t>(strlen(page));

        LOG_INF("charset page %u/%u: %s", _page, (unsigned int) ARRAY_SIZE(debug_pages), page);
#else
        if (!bag.draw(qdb, deck, CONFIG_TK_PLAYBACK_DEPTH_MAX, index, question)) {
            LOG_WRN("deck %u (%s) yielded nothing", deck, tk_deck_name(deck));
            return false;
        }
#endif

        struct tk_question_msg msg = {};

        msg.seq = ++_seq;
        msg.deck = deck;
        msg.len = question.len;

        if (msg.len > sizeof(msg.text)) {
            // qdb enforces this at open() and the layout suite checks every
            // shipped question, so reaching here means a bundle got past both.
            LOG_ERR("question %u is %u bytes", index, question.len);
            return false;
        }

        for (uint16_t i = 0; i < msg.len; i++) {
            msg.text[i] = question.text[i];
        }

        LOG_INF("deck %u %s: %.*s", deck, tk_deck_name(deck), (int) msg.len, msg.text);

        _last_deck = deck;
        _last_was_question = true;

        return zbus_chan_pub(&chan_question, &msg, K_MSEC(100)) == 0;
    }

    bool show_category(uint8_t deck) override
    {
        const char *label = tk_deck_label(deck);

        struct tk_question_msg msg = {};

        msg.seq = ++_seq;
        msg.deck = deck;
        msg.is_category = true;

        while (msg.len < sizeof(msg.text) && label[msg.len] != '\0') {
            msg.text[msg.len] = label[msg.len];
            msg.len++;
        }

        LOG_INF("deck %u %s: showing the name", deck, tk_deck_name(deck));

        _last_deck = deck;
        _last_was_question = false;

        return zbus_chan_pub(&chan_question, &msg, K_MSEC(100)) == 0;
    }

    bool retained_matches(uint8_t deck) const override
    {
        /*
         * Only a question counts. Waking to a deck name means Category was
         * pressed and Next never was, so the panel is mid-conversation with
         * someone and the name has to stay up — but it is not an answer, and
         * the state machine would be wrong to treat it as one.
         *
         * Consulted only from BOOT, which is entered once and never returned
         * to, so this always describes the previous session rather than
         * something this boot drew.
         */
        const tk::Retained &block = tk_retained();

        return tk_retained_survived() && block.showing_question && block.deck == deck;
    }

    /** The seq of the question most recently published. */
    uint32_t last_seq() const { return _seq; }

    /** Continue the sequence across a wake rather than restarting it at 1. */
    void adopt_seq(uint32_t seq) { _seq = seq; }

    /**
     * Record what is now on the glass, and stamp it for the next boot.
     *
     * Called after a render succeeds rather than after a publish: what the
     * next boot needs to know is what the panel is actually showing, and a
     * question that failed to render is not it.
     */
    void remember()
    {
        tk::Retained &block = tk_retained();

        block.deck = _last_deck;
        block.showing_question = _last_was_question;
        block.seq = _seq;

        tk_retained_seal();
    }

private:
    uint32_t _seq = 0;
    uint8_t _last_deck = 0;
    bool _last_was_question = false;
#ifdef CONFIG_TK_DEBUG_CHARSET
    uint8_t _page = 0;
#endif
};

Io io;
tk::AppFsm fsm(io);

} // namespace

int tk_app_init(void)
{
    if (!io.qdb.open(corpus, sizeof(corpus))) {
        LOG_ERR("the embedded corpus is not a valid TKB2 bundle");
        return -EINVAL;
    }

    /*
     * bind() wipes the bag when the bundle it describes is not the one in
     * flash, which on a cold boot is every time: retained_load() has already
     * zeroed the block, so the fingerprint is 0 and matches nothing.
     */
    const bool kept = io.bag.bind(io.qdb);

    if (tk_retained_survived()) {
        io.adopt_seq(tk_retained().seq);
    }

    LOG_INF("corpus: %u questions, %.*s, version %.*s", io.qdb.count(), io.qdb.language_len(),
            io.qdb.language(), io.qdb.version_len(), io.qdb.version());
    LOG_INF("retained state: %s", !tk_retained_survived() ? "cold boot, starting a fresh cycle"
                                  : kept                  ? "kept across the reboot"
                                                          : "discarded, the bundle changed");

    /*
     * Announce the deck the device is on before anything else runs. With a
     * rotary selector this came from reading the pins; with a button it comes
     * from RTC memory, and on a cold boot the zeroed block makes it New
     * People. Either way the machine has a valid deck on its first pass, which
     * is what lets BOOT leave.
     */
    const uint8_t deck = tk_retained().active_deck < TK_DECK_COUNT ? tk_retained().active_deck : 0;

    LOG_INF("active deck: %u %s", deck, tk_deck_name(deck));

    fsm.post_selector(deck, true);

    return 0;
}

void tk_app_post_category(void)
{
    tk::Retained &block = tk_retained();

    /*
     * Advance and wrap. The deck lives here rather than in the state machine
     * because it has to survive a wake, and a wake is a fresh boot: the FSM is
     * told the answer, it does not keep it.
     */
    block.active_deck = (uint8_t) ((block.active_deck + 1) % TK_DECK_COUNT);
    tk_retained_seal();

    LOG_INF("category: deck %u %s", block.active_deck, tk_deck_name(block.active_deck));

    fsm.post_selector(block.active_deck, true);
}

void tk_app_post_next(void)
{
    fsm.post_next();
}

void tk_app_post_render(bool ok, uint32_t seq)
{
    if (seq != io.last_seq()) {
        LOG_WRN("late render for seq %u, waiting on %u — discarded", seq, io.last_seq());
        return;
    }

    if (ok) {
        // The panel's refresh counter has settled by now — the render is what
        // moved it — so this stamp covers that as well as the question.
        io.remember();
    }

    fsm.post_render(ok);
}

void tk_app_run(void)
{
    // Bounded rather than while(changed): a table bug that made two states
    // point at each other would otherwise spin here forever instead of being
    // noticed.
    for (int i = 0; i < 16; i++) {
        const int before = fsm.get_current_state();

        fsm.run();

        if (fsm.get_current_state() == before) {
            return;
        }
    }

    LOG_ERR("state machine did not settle");
}

bool tk_app_needs_timeout(void)
{
    return fsm.current_state_has_timeout();
}

int tk_app_state(void)
{
    return fsm.get_current_state();
}

bool tk_app_is_busy(void)
{
    return fsm.get_current_state() == static_cast<int>(tk::AppFsm::State::REFRESHING);
}

bool tk_app_is_settled(void)
{
    return fsm.get_current_state() == static_cast<int>(tk::AppFsm::State::SHOWING);
}
