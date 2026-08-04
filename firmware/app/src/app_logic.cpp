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

LOG_MODULE_REGISTER(tk_app, LOG_LEVEL_INF);

namespace
{

const uint8_t corpus[] = {
#include "corpus.inc"
};

/*
 * Bag state belongs in RTC slow memory, so a Next press survives the reboot
 * that deep sleep really is. CONFIG_PM is off for the breadboard stage and
 * retained_mem is still unverified on this board, so for now it is ordinary
 * .bss: the cycle resets on every reboot rather than persisting. Moving it is
 * a change of section attribute, not of structure.
 */
tk::Bag::State bag_state;

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

        return zbus_chan_pub(&chan_question, &msg, K_MSEC(100)) == 0;
    }

    bool retained_matches(uint8_t deck) const override
    {
        /*
         * Always false for now. The panel does hold its image without power,
         * but knowing *which* question is on it needs the retained-memory work
         * that comes with deep sleep. Until then every boot draws, which is
         * correct — just not yet free.
         */
        ARG_UNUSED(deck);

        return false;
    }

    /** The seq of the question most recently published. */
    uint32_t last_seq() const { return _seq; }

private:
    uint32_t _seq = 0;
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

    io.bag.bind(io.qdb);

    LOG_INF("corpus: %u questions, %.*s, version %.*s", io.qdb.count(), io.qdb.language_len(),
            io.qdb.language(), io.qdb.version_len(), io.qdb.version());

    return 0;
}

void tk_app_post_selector(uint8_t deck, bool valid)
{
    fsm.post_selector(deck, valid);
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
