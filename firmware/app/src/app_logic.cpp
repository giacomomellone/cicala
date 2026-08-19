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

LOG_MODULE_REGISTER(kveld_app, LOG_LEVEL_INF);

namespace
{

#ifdef CONFIG_KVELD_DEBUG_CORPUS_STORE
void store_compiled_in_corpus()
{
    const int index = kveld_corpus_find(kveld_language());

    if (index < 0) {
        return;
    }

    size_t size = 0;
    const uint8_t *const data = kveld_corpus_data((size_t) index, &size);

    LOG_WRN("CONFIG_KVELD_DEBUG_CORPUS_STORE: writing the compiled-in %s corpus to the filesystem",
            kveld_language());

    (void) kveld_corpus_store(kveld_language(), data, size);
}
#endif

bool open_corpus(kveld::Qdb &qdb)
{
    size_t size = 0;

    /* Prefer a valid stored corpus, with the compiled corpus as fallback. */
    const uint8_t *const stored = kveld_corpus_stored(kveld_language(), &size);

    if (stored != nullptr && qdb.open(stored, size)) {
        return true;
    }

    if (stored != nullptr) {
        LOG_ERR("the stored %s corpus does not parse; using the compiled-in one", kveld_language());
    }

    int index = kveld_corpus_find(kveld_language());

    if (index < 0) {
        LOG_WRN("no corpus for %s; falling back to %s", kveld_language(), kveld_corpus_language(0));
        index = 0;
    }

    const uint8_t *const data = kveld_corpus_data((size_t) index, &size);

    return qdb.open(data, size);
}

/* Keep the no-repeat cycle across deep-sleep restarts. */
kveld::Bag::State &bag_state = kveld_retained().bag;

uint32_t random_u32(void *ctx)
{
    ARG_UNUSED(ctx);

    return sys_rand32_get();
}

#ifdef CONFIG_KVELD_DEBUG_CHARSET
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

class Io : public kveld::AppIo
{
public:
    kveld::Qdb qdb;
    kveld::Bag bag{bag_state, random_u32, nullptr};

    bool draw(uint8_t deck) override
    {
        // Check power before draw() mutates retained bag state.
        if (!refresh_allowed()) {
            return false;
        }

        uint16_t index = 0;
        kveld::Question question;

#ifdef CONFIG_KVELD_DEBUG_CHARSET
        const char *const page = debug_pages[_page];

        _page = (_page + 1) % ARRAY_SIZE(debug_pages);

        question.text = page;
        question.len = static_cast<uint16_t>(strlen(page));

        LOG_INF("charset page %u/%u: %s", _page, (unsigned int) ARRAY_SIZE(debug_pages), page);
#else
        if (!bag.draw(qdb, deck, CONFIG_KVELD_PLAYBACK_DEPTH_MAX, index, question)) {
            LOG_WRN("deck %u (%s) yielded nothing", deck, kveld_deck_name(deck));
            return false;
        }
#endif

        struct kveld_question_msg msg = {};

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

        LOG_INF("deck %u %s: %.*s", deck, kveld_deck_name(deck), (int) msg.len, msg.text);

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

        const char *label = kveld_deck_label(deck);

        struct kveld_question_msg msg = {};

        msg.seq = ++_seq;
        msg.deck = deck;
        msg.kind = KVELD_CARD_CATEGORY;

        while (msg.len < sizeof(msg.text) && label[msg.len] != '\0') {
            msg.text[msg.len] = label[msg.len];
            msg.len++;
        }

        LOG_INF("deck %u %s: showing the name", deck, kveld_deck_name(deck));

        _last_deck = deck;
        _last_was_question = false;

        return zbus_chan_pub(&chan_question, &msg, K_MSEC(100)) == 0;
    }

    bool retained_matches(uint8_t deck) const override
    {
        const kveld::Retained &block = kveld_retained();

        return kveld_retained_survived() && block.showing_question && block.deck == deck;
    }

    bool show_service() override
    {
        if (!refresh_allowed()) {
            return false;
        }

        struct kveld_question_msg msg = {};

        msg.seq = ++_seq;
        msg.deck = _last_deck;
        msg.kind = KVELD_CARD_SERVICE;
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
        kveld::Retained &block = kveld_retained();

        block.deck = _last_deck;
        block.showing_question = _last_was_question;
        block.seq = _seq;

        kveld_retained_seal();
    }

private:
    bool refresh_allowed()
    {
        if (kveld_power_refresh_allowed()) {
            return true;
        }

        LOG_WRN("%u mV is under the %d mV floor; the panel keeps what it has",
                kveld_power_millivolts(), CONFIG_KVELD_REFRESH_MIN_MV);

        kveld_status_note_refresh_blocked();

        return false;
    }

    uint32_t _seq = 0;
    uint8_t _last_deck = 0;
    bool _last_was_question = false;
    char _service_text[CONFIG_KVELD_MAX_QUESTION_BYTES] = {};
    uint16_t _service_len = 0;
#ifdef CONFIG_KVELD_DEBUG_CHARSET
    uint8_t _page = 0;
#endif
};

Io io;
kveld::AppFsm fsm(io);

} // namespace

int kveld_app_init(void)
{
#ifdef CONFIG_KVELD_DEBUG_CORPUS_STORE
    store_compiled_in_corpus();
#endif

    if (!open_corpus(io.qdb)) {
        LOG_ERR("the embedded corpus is not a valid QDB3 bundle");
        return -EINVAL;
    }

    /* bind() resets bag state when the corpus fingerprint changes. */
    const bool kept = io.bag.bind(io.qdb);

    if (kveld_retained_survived()) {
        io.adopt_seq(kveld_retained().seq);
    }

    LOG_INF("corpus: %u questions, %.*s, version %.*s", io.qdb.count(), io.qdb.language_len(),
            io.qdb.language(), io.qdb.version_len(), io.qdb.version());
    LOG_INF("retained state: %s", !kveld_retained_survived() ? "cold boot, starting a fresh cycle"
                                  : kept                     ? "kept across the reboot"
                                                             : "discarded, the bundle changed");

    /* Replay the wake press captured before the input driver started. */
    const enum kveld_wake_source woke_by = kveld_wake_button();

    if (woke_by == KVELD_WAKE_CATEGORY) {
        kveld::Retained &block = kveld_retained();

        block.active_deck = kveld_deck_cycle_next(block.active_deck);
        kveld_retained_seal();

        LOG_INF("woken by Category");
    } else if (woke_by == KVELD_WAKE_NEXT) {
        LOG_INF("woken by Next");

        fsm.post_next();
    }

    const uint8_t stored_deck = kveld_retained().active_deck;
    const uint8_t deck = kveld_deck_on_device(stored_deck) ? stored_deck : 0;

    LOG_INF("active deck: %u %s", deck, kveld_deck_name(deck));

    fsm.post_selector(deck, true);

    return 0;
}

void kveld_app_post_category(void)
{
    kveld::Retained &block = kveld_retained();

    block.active_deck = kveld_deck_cycle_next(block.active_deck);
    kveld_retained_seal();

    LOG_INF("category: deck %u %s", block.active_deck, kveld_deck_name(block.active_deck));

    fsm.post_selector(block.active_deck, true);
}

void kveld_app_post_next(void)
{
    fsm.post_next();
}

void kveld_app_corpus(char *version, size_t version_size, uint16_t *count)
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

void kveld_app_reload_corpus(void)
{
#ifdef CONFIG_KVELD_DEBUG_CORPUS_STORE
    store_compiled_in_corpus();
#endif

    if (!open_corpus(io.qdb)) {
        LOG_ERR("could not reopen the corpus for %s", kveld_language());
        return;
    }

    /* A language change resets bag state through the corpus fingerprint. */
    (void) io.bag.bind(io.qdb);

    LOG_INF("corpus is now %s, %u questions", kveld_language(), io.qdb.count());
}

void kveld_app_post_service(const char *text, uint16_t len)
{
    io.set_service(text, len);
    fsm.post_service();
}

void kveld_app_post_render(bool ok, uint32_t seq)
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

void kveld_app_run(void)
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

bool kveld_app_needs_timeout(void)
{
    return fsm.current_state_has_timeout();
}

int kveld_app_state(void)
{
    return fsm.get_current_state();
}

bool kveld_app_is_busy(void)
{
    return fsm.get_current_state() == static_cast<int>(kveld::AppFsm::State::REFRESHING);
}

bool kveld_app_is_settled(void)
{
    return fsm.get_current_state() == static_cast<int>(kveld::AppFsm::State::SHOWING);
}
