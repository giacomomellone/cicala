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

LOG_MODULE_REGISTER(cicala_app, LOG_LEVEL_INF);

namespace
{

#ifdef CONFIG_CICALA_DEBUG_CORPUS_STORE
void store_compiled_in_corpus()
{
    const int index = cicala_corpus_find(cicala_language());

    if (index < 0) {
        return;
    }

    size_t size = 0;
    const uint8_t *const data = cicala_corpus_data((size_t) index, &size);

    LOG_WRN("CONFIG_CICALA_DEBUG_CORPUS_STORE: writing the compiled-in %s corpus to the filesystem",
            cicala_language());

    (void) cicala_corpus_store(cicala_language(), data, size);
}
#endif

bool open_corpus(cicala::Qdb &qdb)
{
    size_t size = 0;

    /* Prefer a valid stored corpus, with the compiled corpus as fallback. */
    const uint8_t *const stored = cicala_corpus_stored(cicala_language(), &size);

    if (stored != nullptr && qdb.open(stored, size)) {
        return true;
    }

    if (stored != nullptr) {
        LOG_ERR("the stored %s corpus does not parse; using the compiled-in one",
                cicala_language());
    }

    int index = cicala_corpus_find(cicala_language());

    if (index < 0) {
        LOG_WRN("no corpus for %s; falling back to %s", cicala_language(),
                cicala_corpus_language(0));
        index = 0;
    }

    const uint8_t *const data = cicala_corpus_data((size_t) index, &size);

    return qdb.open(data, size);
}

uint32_t random_u32(void *ctx)
{
    ARG_UNUSED(ctx);

    return sys_rand32_get();
}

#ifdef CONFIG_CICALA_DEBUG_CHARSET
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

class Io : public cicala::AppIo
{
public:
    cicala::Qdb qdb;

    bool bind()
    {
        char language[4] = {};
        memcpy(language, qdb.language(), MIN(qdb.language_len(), sizeof(language) - 1));
        const int index = cicala_corpus_find(language);
        _slot = index >= 0 && index < 3 ? index : 0;
        cicala::Bag bag(cicala_retained().bags[_slot], random_u32, nullptr);
        const bool kept = bag.bind(qdb);
        cicala_retained_seal();
        return kept;
    }

    bool draw() override
    {
        if (!begin())
            return false;
        if (_play.menu) {
            if (_play.cursor < 3) {
                _play.draft ^= 1u << _play.cursor;
                return publish(CICALA_CARD_FILTERS);
            }
            _play.permissions = _play.draft;
            _play.menu = false;
            if (_play.len && !(_play.restrictions & ~_play.permissions)) {
                return publish(CICALA_CARD_QUESTION);
            }
        }
        uint16_t index = 0;
        cicala::Question question = {};
        cicala::Bag bag(_bag, random_u32, nullptr);
#ifdef CONFIG_CICALA_DEBUG_CHARSET
        const char *page = debug_pages[_page];
        _page = (_page + 1) % ARRAY_SIZE(debug_pages);
        question.text = page;
        question.len = strlen(page);
#else
        if (!bag.draw(qdb, _play.permissions, index, question)) {
            _play.len = 0;
            _play.restrictions = 0;
            return publish(CICALA_CARD_EMPTY);
        }
#endif
        if (question.len > sizeof(_play.text))
            return false;
        _play.len = question.len;
        memcpy(_play.text, question.text, question.len);
        _play.restrictions = (question.dark ? cicala::kAllowDark : 0) |
                             (question.sexual ? cicala::kAllowSexual : 0) |
                             (question.depth == 3 ? cicala::kAllowHeavy : 0);
        return publish(CICALA_CARD_QUESTION);
    }

    bool show_filters() override
    {
        if (!begin())
            return false;
        if (_play.menu)
            _play.cursor = (_play.cursor + 1) % 4;
        else {
            _play.menu = true;
            _play.cursor = 0;
            _play.draft = _play.permissions;
        }
        return publish(CICALA_CARD_FILTERS);
    }

    bool retained_matches() const override
    {
        const auto &block = cicala_retained();
        return cicala_retained_survived() && block.seq && block.play.kind != CICALA_CARD_SERVICE;
    }

    bool show_service() override
    {
        if (!begin())
            return false;
        _play.menu = false;
        return publish(CICALA_CARD_SERVICE);
    }

    void set_service(const char *text, uint16_t len)
    {
        _service_len = MIN(len, sizeof(_service_text));
        memcpy(_service_text, text, _service_len);
    }

    uint32_t last_seq() const { return _seq; }
    void adopt_seq(uint32_t seq) { _seq = seq; }

    void complete(bool ok) override
    {
        if (ok && _pending) {
            auto &block = cicala_retained();
            // A sync during refresh must not revive indices into the old corpus.
            if (block.bags[_pending_slot].fingerprint == _bag.fingerprint) {
                block.bags[_pending_slot] = _bag;
            }
            block.play = _play;
            block.seq = _seq;
            cicala_retained_seal();
        }
        _pending = false;
    }

private:
    bool begin()
    {
        _pending = false;
        if (!cicala_power_refresh_allowed()) {
            cicala_status_note_refresh_blocked();
            return false;
        }
        _play = cicala_retained().play;
        _bag = cicala_retained().bags[_slot];
        _pending_slot = _slot;
        return true;
    }

    bool publish(uint8_t kind)
    {
        struct cicala_question_msg msg = {};
        msg.seq = ++_seq;
        msg.kind = kind;
        msg.permissions = kind == CICALA_CARD_FILTERS ? _play.draft : _play.permissions;
        msg.cursor = _play.cursor;
        if (kind == CICALA_CARD_QUESTION) {
            msg.len = _play.len;
            memcpy(msg.text, _play.text, msg.len);
        } else if (kind == CICALA_CARD_SERVICE) {
            msg.len = _service_len;
            memcpy(msg.text, _service_text, msg.len);
        }
        _play.kind = kind;
        _pending = zbus_chan_pub(&chan_question, &msg, K_MSEC(100)) == 0;
        return _pending;
    }

    uint32_t _seq = 0;
    uint8_t _slot = 0;
    uint8_t _pending_slot = 0;
    bool _pending = false;
    cicala::PlayState _play = {};
    cicala::Bag::State _bag = {};
    char _service_text[CONFIG_CICALA_MAX_QUESTION_BYTES] = {};
    uint16_t _service_len = 0;
#ifdef CONFIG_CICALA_DEBUG_CHARSET
    uint8_t _page = 0;
#endif
};

Io io;
cicala::AppFsm fsm(io);

} // namespace

int cicala_app_init(void)
{
#ifdef CONFIG_CICALA_DEBUG_CORPUS_STORE
    store_compiled_in_corpus();
#endif

    if (!open_corpus(io.qdb)) {
        LOG_ERR("the embedded corpus is not a valid QDB4 bundle");
        return -EINVAL;
    }

    /* bind() resets bag state when the corpus fingerprint changes. */
    const bool kept = io.bind();

    if (cicala_retained_survived()) {
        io.adopt_seq(cicala_retained().seq);
    }

    LOG_INF("corpus: %u questions, %.*s, version %.*s", io.qdb.count(), io.qdb.language_len(),
            io.qdb.language(), io.qdb.version_len(), io.qdb.version());
    LOG_INF("retained state: %s", !cicala_retained_survived() ? "cold boot, starting a fresh cycle"
                                  : kept                      ? "kept across the reboot"
                                                              : "discarded, the bundle changed");

    /* Replay the wake press captured before the input driver started. */
    const enum cicala_wake_source woke_by = cicala_wake_button();

    if (woke_by == CICALA_WAKE_CATEGORY) {
        fsm.post_filters();
    } else if (woke_by == CICALA_WAKE_NEXT) {
        fsm.post_next();
    }
    fsm.post_ready();

    return 0;
}

void cicala_app_post_filters(void)
{
    fsm.post_filters();
}

void cicala_app_post_next(void)
{
    fsm.post_next();
}

void cicala_app_corpus(char *version, size_t version_size, uint16_t *count)
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

void cicala_app_reload_corpus(void)
{
#ifdef CONFIG_CICALA_DEBUG_CORPUS_STORE
    store_compiled_in_corpus();
#endif

    if (!open_corpus(io.qdb)) {
        LOG_ERR("could not reopen the corpus for %s", cicala_language());
        return;
    }

    /* A language change resets bag state through the corpus fingerprint. */
    (void) io.bind();

    LOG_INF("corpus is now %s, %u questions", cicala_language(), io.qdb.count());
}

void cicala_app_post_service(const char *text, uint16_t len)
{
    io.set_service(text, len);
    fsm.post_service();
}

void cicala_app_post_render(bool ok, uint32_t seq)
{
    if (seq != io.last_seq()) {
        LOG_WRN("late render for seq %u, waiting on %u — discarded", seq, io.last_seq());
        return;
    }

    fsm.post_render(ok);
}

void cicala_app_run(void)
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

bool cicala_app_needs_timeout(void)
{
    return fsm.current_state_has_timeout();
}

int cicala_app_state(void)
{
    return fsm.get_current_state();
}

bool cicala_app_is_busy(void)
{
    return fsm.get_current_state() == static_cast<int>(cicala::AppFsm::State::REFRESHING);
}

bool cicala_app_is_settled(void)
{
    return fsm.get_current_state() == static_cast<int>(cicala::AppFsm::State::SHOWING);
}
