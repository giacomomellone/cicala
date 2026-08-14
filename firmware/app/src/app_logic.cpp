/* The decision maker, wired to the question store and the display channel. */

#include "app_logic.h"

#include <string.h>

#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

#include "app_fsm.hpp"
#include "channels.h"
#include "corpus.h"
#include "language.h"
#include "power.h"
#include "qdb.hpp"
#include "retained_block.hpp"
#include "sleep.h"
#include "status.h"

LOG_MODULE_REGISTER(tk_app, LOG_LEVEL_INF);

namespace
{

#ifdef CONFIG_TK_DEBUG_CORPUS_STORE
void store_compiled_in_corpus()
{
    const int index = tk_corpus_find(tk_language());

    if (index < 0) {
        return;
    }

    size_t size = 0;
    const uint8_t *const data = tk_corpus_data((size_t) index, &size);

    LOG_WRN("CONFIG_TK_DEBUG_CORPUS_STORE: writing the compiled-in %s corpus to the filesystem",
            tk_language());

    (void) tk_corpus_store(tk_language(), data, size);
}
#endif

bool open_corpus(tk::Qdb &qdb)
{
    size_t size = 0;

    /* Prefer a valid stored corpus, with the compiled corpus as fallback. */
    const uint8_t *const stored = tk_corpus_stored(tk_language(), &size);

    if (stored != nullptr && qdb.open(stored, size)) {
        return true;
    }

    if (stored != nullptr) {
        LOG_ERR("the stored %s corpus does not parse; using the compiled-in one", tk_language());
    }

    int index = tk_corpus_find(tk_language());

    if (index < 0) {
        LOG_WRN("no corpus for %s; falling back to %s", tk_language(), tk_corpus_language(0));
        index = 0;
    }

    const uint8_t *const data = tk_corpus_data((size_t) index, &size);

    return qdb.open(data, size);
}

/* Keep the no-repeat cycle across deep-sleep restarts. */
tk::Bag::State &bag_state = tk_retained().bag;

uint32_t random_u32(void *ctx)
{
    ARG_UNUSED(ctx);

    return sys_rand32_get();
}

#ifdef CONFIG_TK_DEBUG_CHARSET
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

class Io : public tk::AppIo
{
public:
    tk::Qdb qdb;
    tk::Bag bag{bag_state, random_u32, nullptr};

    bool draw(uint8_t deck) override
    {
        // Check power before draw() mutates retained bag state.
        if (!refresh_allowed()) {
            return false;
        }

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
            // Keep an invalid stored bundle from reaching the panel.
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
        // Allocate a sequence number only for a queued card.
        if (!refresh_allowed()) {
            return false;
        }

        const char *label = tk_deck_label(deck);

        struct tk_question_msg msg = {};

        msg.seq = ++_seq;
        msg.deck = deck;
        msg.kind = TK_CARD_CATEGORY;

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
        const tk::Retained &block = tk_retained();

        return tk_retained_survived() && block.showing_question && block.deck == deck;
    }

    bool show_service() override
    {
        if (!refresh_allowed()) {
            return false;
        }

        struct tk_question_msg msg = {};

        msg.seq = ++_seq;
        msg.deck = _last_deck;
        msg.kind = TK_CARD_SERVICE;
        msg.len = _service_len;

        for (uint16_t i = 0; i < _service_len; i++) {
            msg.text[i] = _service_text[i];
        }

        LOG_INF("service card: %.*s", (int) msg.len, msg.text);

        // Service cards never satisfy retained_matches().
        _last_was_question = false;

        return zbus_chan_pub(&chan_question, &msg, K_MSEC(100)) == 0;
    }

    /* Hold the portal's text until the machine reaches SERVICE. */
    void set_service(const char *text, uint16_t len)
    {
        _service_len = len > sizeof(_service_text) ? sizeof(_service_text) : len;

        for (uint16_t i = 0; i < _service_len; i++) {
            _service_text[i] = text[i];
        }
    }

    uint32_t last_seq() const { return _seq; }

    void adopt_seq(uint32_t seq) { _seq = seq; }

    void remember()
    {
        tk::Retained &block = tk_retained();

        block.deck = _last_deck;
        block.showing_question = _last_was_question;
        block.seq = _seq;

        tk_retained_seal();
    }

private:
    bool refresh_allowed()
    {
        if (tk_power_refresh_allowed()) {
            return true;
        }

        LOG_WRN("%u mV is under the %d mV floor; the panel keeps what it has",
                tk_power_millivolts(), CONFIG_TK_REFRESH_MIN_MV);

        tk_status_note_refresh_blocked();

        return false;
    }

    uint32_t _seq = 0;
    uint8_t _last_deck = 0;
    bool _last_was_question = false;
    char _service_text[CONFIG_TK_MAX_QUESTION_BYTES] = {};
    uint16_t _service_len = 0;
#ifdef CONFIG_TK_DEBUG_CHARSET
    uint8_t _page = 0;
#endif
};

Io io;
tk::AppFsm fsm(io);

} // namespace

int tk_app_init(void)
{
#ifdef CONFIG_TK_DEBUG_CORPUS_STORE
    store_compiled_in_corpus();
#endif

    if (!open_corpus(io.qdb)) {
        LOG_ERR("the embedded corpus is not a valid QDB2 bundle");
        return -EINVAL;
    }

    /* bind() resets bag state when the corpus fingerprint changes. */
    const bool kept = io.bag.bind(io.qdb);

    if (tk_retained_survived()) {
        io.adopt_seq(tk_retained().seq);
    }

    LOG_INF("corpus: %u questions, %.*s, version %.*s", io.qdb.count(), io.qdb.language_len(),
            io.qdb.language(), io.qdb.version_len(), io.qdb.version());
    LOG_INF("retained state: %s", !tk_retained_survived() ? "cold boot, starting a fresh cycle"
                                  : kept                  ? "kept across the reboot"
                                                          : "discarded, the bundle changed");

    /* Replay the wake press captured before the input driver started. */
    const enum tk_wake_source woke_by = tk_wake_button();

    if (woke_by == TK_WAKE_CATEGORY) {
        tk::Retained &block = tk_retained();

        block.active_deck = (uint8_t) ((block.active_deck + 1) % TK_DECK_COUNT);
        tk_retained_seal();

        LOG_INF("woken by Category");
    } else if (woke_by == TK_WAKE_NEXT) {
        LOG_INF("woken by Next");

        fsm.post_next();
    }

    const uint8_t deck = tk_retained().active_deck < TK_DECK_COUNT ? tk_retained().active_deck : 0;

    LOG_INF("active deck: %u %s", deck, tk_deck_name(deck));

    fsm.post_selector(deck, true);

    return 0;
}

void tk_app_post_category(void)
{
    tk::Retained &block = tk_retained();

    block.active_deck = (uint8_t) ((block.active_deck + 1) % TK_DECK_COUNT);
    tk_retained_seal();

    LOG_INF("category: deck %u %s", block.active_deck, tk_deck_name(block.active_deck));

    fsm.post_selector(block.active_deck, true);
}

void tk_app_post_next(void)
{
    fsm.post_next();
}

void tk_app_corpus(char *version, size_t version_size, uint16_t *count)
{
    *count = io.qdb.count();

    const char *const v = io.qdb.version();
    size_t n = io.qdb.version_len();

    if (n >= version_size) {
        n = version_size - 1;
    }

    for (size_t i = 0; i < n; i++) {
        version[i] = v[i];
    }

    version[n] = '\0';
}

void tk_app_reload_corpus(void)
{
#ifdef CONFIG_TK_DEBUG_CORPUS_STORE
    store_compiled_in_corpus();
#endif

    if (!open_corpus(io.qdb)) {
        LOG_ERR("could not reopen the corpus for %s", tk_language());
        return;
    }

    /* A language change resets bag state through the corpus fingerprint. */
    (void) io.bag.bind(io.qdb);

    LOG_INF("corpus is now %s, %u questions", tk_language(), io.qdb.count());
}

void tk_app_post_service(const char *text, uint16_t len)
{
    io.set_service(text, len);
    fsm.post_service();
}

void tk_app_post_render(bool ok, uint32_t seq)
{
    if (seq != io.last_seq()) {
        LOG_WRN("late render for seq %u, waiting on %u — discarded", seq, io.last_seq());
        return;
    }

    if (ok) {
        // Seal after the render updates both card and refresh state.
        io.remember();
    }

    fsm.post_render(ok);
}

void tk_app_run(void)
{
    // Bound one event to the number of application states.
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
