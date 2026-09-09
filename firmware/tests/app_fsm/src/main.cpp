#include <zephyr/ztest.h>
#include "app_fsm.hpp"
using namespace cicala;
using State = AppFsm::State;
namespace
{
class FakeIo : public AppIo
{
public:
    int draws = 0, menus = 0, services = 0, commits = 0, rollbacks = 0;
    bool succeeds = true, retained = false;
    bool draw() override
    {
        draws++;
        return succeeds;
    }
    bool show_filters() override
    {
        menus++;
        return succeeds;
    }
    bool show_service() override
    {
        services++;
        return succeeds;
    }
    bool retained_matches() const override { return retained; }
    void complete(bool ok) override
    {
        if (ok)
            commits++;
        else
            rollbacks++;
    }
};
class TestFsm : public AppFsm
{
public:
    using AppFsm::AppFsm;
    int64_t clock = 0;

protected:
    int64_t now_ms() const override { return clock; }
};
void settle(TestFsm &fsm)
{
    for (int i = 0; i < 16; i++) {
        int before = fsm.get_current_state();
        fsm.run();
        if (before == fsm.get_current_state())
            return;
    }
    zassert_unreachable();
}
void boot(TestFsm &fsm)
{
    fsm.post_ready();
    settle(fsm);
    fsm.post_render(true);
    settle(fsm);
}
} // namespace
ZTEST_SUITE(cicala_app_fsm, NULL, NULL, NULL, NULL, NULL);
ZTEST(cicala_app_fsm, test_cold_boot_draws_and_commits_only_after_render)
{
    FakeIo io;
    TestFsm fsm(io);
    settle(fsm);
    zassert_equal(io.draws, 0);
    fsm.post_ready();
    settle(fsm);
    zassert_equal(io.draws, 1);
    zassert_equal(io.commits, 0);
    fsm.post_render(true);
    settle(fsm);
    zassert_equal(io.commits, 1);
}
ZTEST(cicala_app_fsm, test_retained_view_needs_no_refresh)
{
    FakeIo io;
    io.retained = true;
    TestFsm fsm(io);
    fsm.post_ready();
    settle(fsm);
    zassert_equal(io.draws, 0);
    zassert_equal(io.menus, 0);
    zassert_equal(fsm.get_current_state(), STATE(SHOWING));
}
ZTEST(cicala_app_fsm, test_wake_inputs_are_replayed_once)
{
    FakeIo io;
    io.retained = true;
    TestFsm fsm(io);
    fsm.post_filters();
    boot(fsm);
    zassert_equal(io.menus, 1);
    zassert_equal(io.draws, 0);
    fsm.post_next();
    settle(fsm);
    zassert_equal(io.draws, 1);
}
ZTEST(cicala_app_fsm, test_both_inputs_during_refresh_are_dropped)
{
    FakeIo io;
    TestFsm fsm(io);
    fsm.post_ready();
    settle(fsm);
    fsm.post_filters();
    fsm.post_next();
    fsm.post_render(true);
    settle(fsm);
    zassert_equal(io.draws, 1);
    zassert_equal(io.menus, 0);
}
ZTEST(cicala_app_fsm, test_failed_queue_or_render_rolls_back_without_retry_loop)
{
    FakeIo io;
    TestFsm fsm(io);
    boot(fsm);
    io.succeeds = false;
    fsm.post_filters();
    settle(fsm);
    zassert_equal(io.menus, 1);
    zassert_equal(io.rollbacks, 1);
    io.succeeds = true;
    fsm.post_filters();
    settle(fsm);
    fsm.post_render(false);
    settle(fsm);
    zassert_equal(io.rollbacks, 2);
    zassert_equal(io.commits, 1);
    fsm.post_next();
    settle(fsm);
    zassert_equal(io.draws, 2);
}
ZTEST(cicala_app_fsm, test_timeout_rolls_back_and_late_completion_is_ignored)
{
    FakeIo io;
    TestFsm fsm(io);
    fsm.post_ready();
    settle(fsm);
    fsm.clock += CONFIG_CICALA_REFRESH_TIMEOUT_MS;
    settle(fsm);
    zassert_equal(io.rollbacks, 1);
    zassert_equal(io.commits, 0);
    fsm.post_render(true);
    settle(fsm);
    zassert_equal(io.commits, 0);
    fsm.post_next();
    settle(fsm);
    zassert_true(fsm.current_state_has_timeout());
    fsm.post_render(true);
    settle(fsm);
    zassert_equal(io.commits, 1);
}
ZTEST(cicala_app_fsm, test_service_arriving_during_refresh_waits)
{
    FakeIo io;
    TestFsm fsm(io);
    fsm.post_ready();
    settle(fsm);
    fsm.post_service();
    settle(fsm);
    zassert_equal(io.services, 0);
    fsm.post_render(true);
    settle(fsm);
    zassert_equal(io.services, 1);
    fsm.post_render(true);
    settle(fsm);
    fsm.post_filters();
    settle(fsm);
    zassert_equal(io.menus, 1);
}
