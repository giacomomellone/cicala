#include "session.hpp"

#include <string.h>

namespace cicala
{
bool Session::bindCorpus(const Qdb &corpus, Bag::State &bag)
{
    if (_busy || !corpus.is_open())
        return false;
    _corpus = &corpus;
    _bag = &bag;
    Bag selection(bag, _random, _context);
    selection.bind(corpus);
    return true;
}

bool Session::begin()
{
    if (_busy || !_corpus || !_bag)
        return false;
    _view.play = _committed;
    _stagedBag = *_bag;
    return true;
}

const PendingView *Session::finish(ViewKind kind)
{
    _view.play.kind = static_cast<uint8_t>(kind);
    if (++_sequence == 0)
        ++_sequence;
    _view.token = _sequence;
    _busy = true;
    return &_view;
}

const PendingView *Session::draw()
{
    uint16_t index = 0;
    Question question{};
    Bag selection(_stagedBag, _random, _context);
    if (!selection.draw(*_corpus, _view.play.permissions, index, question)) {
        _view.play.len = 0;
        _view.play.restrictions = 0;
        return finish(ViewKind::Empty);
    }
    if (question.len > kMaxQuestionBytes)
        return nullptr;
    _view.play.len = question.len;
    memcpy(_view.play.text, question.text, question.len);
    _view.play.restrictions = (question.dark ? kAllowDark : 0) |
                              (question.sexual ? kAllowSexual : 0) |
                              (question.depth == 3 ? kAllowHeavy : 0);
    return finish(ViewKind::Question);
}

const PendingView *Session::prepare(Action action)
{
    if (action == Action::CancelFilters && !_committed.menu)
        return nullptr;
    if (!begin())
        return nullptr;
    auto &play = _view.play;
    if (action == Action::CancelFilters) {
        play.menu = false;
        play.draft = play.permissions;
        play.cursor = 0;
        return finish(play.len ? ViewKind::Question : ViewKind::Empty);
    }
    if (action == Action::NewSession) {
        play = {};
        _stagedBag = {};
        Bag selection(_stagedBag, _random, _context);
        selection.bind(*_corpus);
        return draw();
    }
    if (action == Action::Filters) {
        if (play.menu)
            play.cursor = (play.cursor + 1) % 4;
        else {
            play.menu = true;
            play.cursor = 0;
            play.draft = play.permissions;
        }
        return finish(ViewKind::Filters);
    }
    if (play.menu)
        return activateFilter();
    return draw();
}

const PendingView *Session::prepareFilterRow(uint8_t row)
{
    if (row > 3 || !_committed.menu || !begin())
        return nullptr;
    _view.play.cursor = row;
    return activateFilter();
}

const PendingView *Session::activateFilter()
{
    auto &play = _view.play;
    if (play.cursor < 3) {
        play.draft ^= 1u << play.cursor;
        return finish(ViewKind::Filters);
    }
    play.permissions = play.draft;
    play.menu = false;
    if (play.len && !(play.restrictions & ~play.permissions))
        return finish(ViewKind::Question);
    return draw();
}

const PendingView *Session::prepareMessage(ViewKind kind, const char *text, size_t len)
{
    if (len > kMaxQuestionBytes || (len && !text) || !begin())
        return nullptr;
    _view.play.menu = false;
    // A service card must not replace the question snapshot used on return.
    if (kind != ViewKind::Service) {
        _view.play.len = static_cast<uint16_t>(len);
        if (len)
            memcpy(_view.play.text, text, len);
        _view.play.restrictions = 0;
    }
    return finish(kind);
}

bool Session::complete(uint32_t token, RenderResult result)
{
    if (!_busy || token != _view.token)
        return false;
    _busy = false;
    if (result != RenderResult::Complete)
        return false;
    if (_bag->fingerprint == _stagedBag.fingerprint)
        *_bag = _stagedBag;
    _committed = _view.play;
    return true;
}
} // namespace cicala
