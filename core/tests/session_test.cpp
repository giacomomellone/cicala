#include "bundle.hpp"
#include "snapshot.hpp"
#include "selection_fixture.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x);                           \
            std::abort();                                                                          \
        }                                                                                          \
    } while (0)

using namespace cicala;

namespace
{
uint32_t zero(void *)
{
    return 0;
}
struct Fixture {
    uint8_t bytes[4096] = {'Q', 'D', 'B', '4', 1, '1', 2, 'e', 'n', 0, 0};
    size_t size = 11;
    void add(uint8_t depth, uint8_t permissions = 0, uint8_t forms = 0)
    {
        constexpr char text[] = "When did you last sing out loud?";
        bytes[size++] =
            (depth - 1) | (permissions & kAllowSexual ? 4 : 0) | (permissions & kAllowDark ? 8 : 0);
        bytes[size++] = forms;
        bytes[size++] = sizeof(text) - 1;
        bytes[size++] = 0;
        memcpy(bytes + size, text, sizeof(text) - 1);
        size += sizeof(text) - 1;
        ++bytes[9];
    }
};
void apply(Session &s, Action action)
{
    const auto *view = s.prepare(action);
    CHECK(view);
    CHECK(s.complete(view->token, RenderResult::Complete));
}
void transactions()
{
    Fixture f;
    f.add(1);
    f.add(2);
    f.add(3);
    Qdb q;
    CHECK(q.open(f.bytes, f.size));
    PlayState play{};
    Bag::State bag{};
    Session s(play, zero, nullptr);
    CHECK(s.bindCorpus(q, bag));
    auto *v = s.prepare(Action::Next);
    CHECK(v);
    const auto token = v->token;
    CHECK(!play.len && !bag.drawn[0]);
    CHECK(!s.prepare(Action::Next));
    CHECK(!s.bindCorpus(q, bag));
    CHECK(!s.complete(token + 1, RenderResult::Complete));
    CHECK(s.busy());
    CHECK(!s.complete(token, RenderResult::Timeout));
    CHECK(!play.len && !bag.drawn[0]);
    apply(s, Action::Next);
    const auto seen = bag.drawn[0];
    apply(s, Action::Filters);
    CHECK(play.menu && play.draft == 0);
    v = s.prepare(Action::Next);
    CHECK(v && v->play.draft == kAllowDark);
    CHECK(!s.complete(v->token, RenderResult::Failed));
    CHECK(play.draft == 0);
    apply(s, Action::Next);
    CHECK(play.permissions == 0 && play.draft == kAllowDark);
    apply(s, Action::Filters);
    apply(s, Action::Filters);
    apply(s, Action::Filters);
    apply(s, Action::Next);
    CHECK(!play.menu && play.permissions == kAllowDark);
    CHECK(bag.drawn[0] == seen);
    apply(s, Action::NewSession);
    CHECK(play.permissions == 0 && !play.menu && play.len);
    CHECK(!s.complete(token, RenderResult::Complete));
    // A copied question survives destruction of the source text.
    const char first = play.text[0];
    memset(f.bytes, 0, f.size);
    CHECK(play.text[0] == first);
}
void filter_rows()
{
    Fixture f;
    f.add(2);
    Qdb q;
    CHECK(q.open(f.bytes, f.size));
    PlayState play{};
    Bag::State bag{};
    Session s(play, zero, nullptr);
    CHECK(s.bindCorpus(q, bag));
    apply(s, Action::Next);
    CHECK(!s.prepareFilterRow(1));
    apply(s, Action::Filters);
    CHECK(!s.prepareFilterRow(4));
    auto *v = s.prepareFilterRow(2);
    CHECK(v && v->play.cursor == 2 && v->play.draft == kAllowHeavy);
    CHECK(!s.prepareFilterRow(0));
    CHECK(!s.complete(v->token, RenderResult::Failed));
    CHECK(play.cursor == 0 && play.draft == 0 && play.permissions == 0);
    v = s.prepareFilterRow(1);
    CHECK(v && s.complete(v->token, RenderResult::Complete));
    CHECK(play.cursor == 1 && play.draft == kAllowSexual && play.permissions == 0);
    const auto seen = bag.drawn[0];
    v = s.prepareFilterRow(3);
    CHECK(v && !s.complete(v->token, RenderResult::Timeout));
    CHECK(play.menu && play.permissions == 0 && bag.drawn[0] == seen);
    v = s.prepareFilterRow(3);
    CHECK(v && s.complete(v->token, RenderResult::Complete));
    CHECK(!play.menu && play.permissions == kAllowSexual && bag.drawn[0] == seen);
}
void selection()
{
    Fixture f;
    for (int p = 0; p < 8; ++p)
        f.add(p & kAllowHeavy ? 3 : 2, p);
    Qdb q;
    CHECK(q.open(f.bytes, f.size));
    for (int allowed = 0; allowed < 8; ++allowed)
        for (int required = 0; required < 8; ++required)
            CHECK(q.is_eligible(required, allowed) == ((required & ~allowed) == 0));
    Bag::State state{};
    Bag b(state, zero, nullptr);
    b.bind(q);
    Question chosen{};
    uint16_t index;
    for (int i = 0; i < 8; ++i)
        CHECK(b.draw(q, 7, index, chosen));
    CHECK(b.drawn_count() == 8);
    CHECK(b.draw(q, 0, index, chosen) && index == 0);
    CHECK((state.drawn[0] & 0xfe) == 0xfe);
    Fixture empty;
    CHECK(q.open(empty.bytes, empty.size));
    b.bind(q);
    CHECK(!b.draw(q, 0, index, chosen));
    empty.add(3);
    CHECK(q.open(empty.bytes, empty.size));
    b.bind(q);
    CHECK(!b.draw(q, 0, index, chosen));
    CHECK(b.draw(q, kAllowHeavy, index, chosen));
    CHECK(b.draw(q, kAllowHeavy, index, chosen));
}
void snapshots()
{
    Snapshot a{};
    a.generation = 7;
    a.play.permissions = kAllowHeavy;
    a.play.menu = true;
    a.play.cursor = 2;
    a.play.draft = 7;
    a.bag.fingerprint = 22;
    a.bag.recent_len = 1;
    a.bag.recent_next = 1;
    a.bag.recent[0] = 3;
    a.bag.drawn[0] = 8;
    uint8_t bytes[kSnapshotBytes];
    CHECK(snapshot_encode(a, bytes, sizeof(bytes)));
    Snapshot b{};
    CHECK(snapshot_decode(bytes, sizeof(bytes), b));
    CHECK(b.generation == 7 && b.play.draft == 7 && b.bag.drawn[0] == 8);
    for (size_t i = 0; i < sizeof(bytes); ++i) {
        bytes[i] ^= 1;
        CHECK(!snapshot_decode(bytes, sizeof(bytes), b));
        CHECK(b.generation == 7);
        bytes[i] ^= 1;
    }
    CHECK(!snapshot_decode(bytes, sizeof(bytes) - 1, b));
    a.play.cursor = 9;
    CHECK(!snapshot_encode(a, bytes, sizeof(bytes)));
    CHECK(generation_newer(1, 0xffffffffu));
    CHECK(!generation_newer(0xffffffffu, 1));
}
void rebind()
{
    Fixture f;
    f.add(2);
    Qdb q;
    CHECK(q.open(f.bytes, f.size));
    PlayState play{};
    Bag::State bag{};
    Session s(play, zero, nullptr);
    CHECK(s.bindCorpus(q, bag));
    apply(s, Action::Next);
    CHECK(bag.drawn[0]);
    f.add(1);
    CHECK(q.open(f.bytes, f.size));
    CHECK(s.bindCorpus(q, bag));
    CHECK(!bag.drawn[0] && play.len);
    auto *v = s.prepare(Action::Next);
    CHECK(v);
    const auto token = v->token;
    s.cancel();
    CHECK(s.bindCorpus(q, bag));
    CHECK(!s.complete(token, RenderResult::Complete));
}
void shared_selection()
{
    Fixture f;
    for (const auto &row : shared_fixture::questions)
        f.add(row[0], row[1], row[2]);
    Qdb q;
    CHECK(q.open(f.bytes, f.size));
    for (unsigned permissions = 0; permissions < 8; ++permissions) {
        unsigned eligible = 0;
        for (unsigned i = 0; i < q.count(); ++i)
            if (q.is_eligible(i, permissions))
                eligible |= 1u << i;
        CHECK(eligible == shared_fixture::eligible[permissions]);
    }
    Bag::State state{};
    Bag bag(state, zero, nullptr);
    bag.bind(q);
    for (unsigned step = 0; step < sizeof(shared_fixture::draws) / sizeof(unsigned); ++step) {
        Question question{};
        uint16_t index;
        CHECK(bag.draw(q, shared_fixture::permissions[step], index, question));
        CHECK(index == shared_fixture::draws[step]);
    }
    CHECK(state.drawn[0] == shared_fixture::seen);
}
} // namespace

int main()
{
    transactions();
    filter_rows();
    selection();
    snapshots();
    rebind();
    shared_selection();
    std::puts("Cicala core tests passed");
}
