#pragma once

#include "qdb.hpp"

namespace cicala
{
enum class ViewKind : uint8_t { Question = 0, Filters, Service, Empty };
enum class Action { Next, Filters, NewSession, CancelFilters };
enum class RenderResult { Complete, Failed, Timeout };

/** Stable field order also used by the existing Zephyr RTC adapter. */
struct PlayState {
    uint8_t permissions;
    uint8_t draft;
    uint8_t cursor;
    bool menu;
    uint8_t kind;
    uint8_t restrictions;
    uint16_t len;
    char text[kMaxQuestionBytes];
};

struct PendingView {
    uint32_t token;
    PlayState play;
};

/** Single-owner, allocation-free transactions. Referenced state and corpus must outlive this object. */
class Session
{
public:
    Session(PlayState &committed, Bag::RandFn random, void *context)
        : _committed(committed), _random(random), _context(context)
    {
    }

    /** Refuses rebinding during a pending transaction. Clears incompatible bag indices. */
    bool bindCorpus(const Qdb &corpus, Bag::State &bag);
    const PendingView *prepare(Action action);
    /** Activate a specific filter row (0..2 toggles, 3 applies Done). Requires an open menu. */
    const PendingView *prepareFilterRow(uint8_t row);
    /** Service/debug cards use the same commit protocol as question draws. */
    const PendingView *prepareMessage(ViewKind kind, const char *text, size_t len);
    bool complete(uint32_t token, RenderResult result);
    void cancel() { _busy = false; }
    bool busy() const { return _busy; }
    const PlayState &state() const { return _committed; }
    const PendingView &pending() const { return _view; }
    uint32_t sequence() const { return _sequence; }
    void adoptSequence(uint32_t sequence)
    {
        if (!_busy)
            _sequence = sequence;
    }

private:
    bool begin();
    const PendingView *finish(ViewKind kind);
    const PendingView *draw();
    const PendingView *activateFilter();
    PlayState &_committed;
    Bag::RandFn _random;
    void *_context;
    const Qdb *_corpus = nullptr;
    Bag::State *_bag = nullptr;
    Bag::State _stagedBag{};
    PendingView _view{};
    uint32_t _sequence = 0;
    bool _busy = false;
};
} // namespace cicala
