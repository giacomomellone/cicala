#include "app_fsm.hpp"
#include <zephyr/sys/util.h>
namespace cicala
{
// clang-format off
const Fsm::StateTransition AppFsm::_transitions[] = {
    {STATE(BOOT), TRANSITION(REPEAT), STATE(BOOT), 0},
    {STATE(BOOT), TRANSITION(RETAINED), STATE(SHOWING), 0},
    {STATE(BOOT), TRANSITION(REDRAW), STATE(DRAWING), 0},
    {STATE(BOOT), TRANSITION(FILTERS), STATE(FILTERS), 0},
    {STATE(SHOWING), TRANSITION(REPEAT), STATE(SHOWING), 0},
    {STATE(SHOWING), TRANSITION(REDRAW), STATE(DRAWING), 0},
    {STATE(SHOWING), TRANSITION(FILTERS), STATE(FILTERS), 0},
    {STATE(SHOWING), TRANSITION(SERVICE), STATE(SERVICE), 0},
    {STATE(FILTERS), TRANSITION(CONTINUE), STATE(REFRESHING), 0},
    {STATE(FILTERS), TRANSITION(FAILED), STATE(FAIL), 0},
    {STATE(DRAWING), TRANSITION(CONTINUE), STATE(REFRESHING), 0},
    {STATE(DRAWING), TRANSITION(FAILED), STATE(FAIL), 0},
    {STATE(SERVICE), TRANSITION(CONTINUE), STATE(REFRESHING), 0},
    {STATE(SERVICE), TRANSITION(FAILED), STATE(FAIL), 0},
    {STATE(REFRESHING), TRANSITION(REPEAT), STATE(REFRESHING), CONFIG_CICALA_REFRESH_TIMEOUT_MS},
    {STATE(REFRESHING), TRANSITION(CONTINUE), STATE(SHOWING), 0},
    {STATE(REFRESHING), TRANSITION(FAILED), STATE(FAIL), 0},
    {STATE(FAIL), TRANSITION(CONTINUE), STATE(SHOWING), 0},
};
// clang-format on
AppFsm::AppFsm(AppIo &io) : Fsm(_transitions, ARRAY_SIZE(_transitions), STATE(BOOT)), _io(io) {}
int AppFsm::get_fail_state() const
{
    return STATE(FAIL);
}
bool AppFsm::accepts_input() const
{
    return get_current_state() == STATE(BOOT) || get_current_state() == STATE(SHOWING);
}
void AppFsm::post_filters()
{
    if (accepts_input())
        _filters_pending = true;
}
void AppFsm::post_next()
{
    if (accepts_input())
        _next_pending = true;
}
void AppFsm::post_render(bool ok)
{
    if (get_current_state() != STATE(REFRESHING))
        return;
    _render_pending = true;
    _render_ok = ok;
}
void AppFsm::on_enter_state(int state)
{
    if (state == STATE(FAIL)) {
        _io.complete(false);
        return;
    }
    if (state == STATE(DRAWING) || state == STATE(FILTERS) || state == STATE(SERVICE)) {
        _render_pending = false;
        _next_pending = _filters_pending = false;
        _queued = state == STATE(DRAWING)   ? _io.draw()
                  : state == STATE(FILTERS) ? _io.show_filters()
                                            : _io.show_service();
    }
}
int AppFsm::handle_current_state()
{
    const int state = get_current_state();
    if (state == STATE(FAIL))
        return TRANSITION(CONTINUE);
    if (state == STATE(REFRESHING)) {
        if (!_render_pending)
            return TRANSITION(REPEAT);
        _render_pending = false;
        if (_render_ok) {
            _io.complete(true);
            return TRANSITION(CONTINUE);
        }
        return TRANSITION(FAILED);
    }
    if (state == STATE(DRAWING) || state == STATE(FILTERS) || state == STATE(SERVICE))
        return _queued ? TRANSITION(CONTINUE) : TRANSITION(FAILED);
    if (!_ready)
        return TRANSITION(REPEAT);
    if (state == STATE(SHOWING) && _service_pending) {
        _service_pending = false;
        return TRANSITION(SERVICE);
    }
    if (_filters_pending) {
        _filters_pending = false;
        return TRANSITION(FILTERS);
    }
    if (_next_pending) {
        _next_pending = false;
        return TRANSITION(REDRAW);
    }
    if (state == STATE(BOOT))
        return _io.retained_matches() ? TRANSITION(RETAINED) : TRANSITION(REDRAW);
    return TRANSITION(REPEAT);
}
} // namespace cicala
