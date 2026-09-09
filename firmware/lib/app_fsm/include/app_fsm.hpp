/* Serializes tabletop effects. State is committed only after a successful render. */
#pragma once
#include "fsm.hpp"
namespace cicala
{
class AppIo
{
public:
    virtual ~AppIo() = default;
    virtual bool draw() = 0;
    virtual bool show_filters() = 0;
    virtual bool show_service() = 0;
    virtual bool retained_matches() const = 0;
    virtual void complete(bool ok) = 0;
};
class AppFsm : public Fsm
{
public:
    enum class State { BOOT = 0, FILTERS, SHOWING, DRAWING, REFRESHING, SERVICE, FAIL };
    enum class Transition { REPEAT = 0, CONTINUE, REDRAW, FILTERS, RETAINED, SERVICE, FAILED };
    explicit AppFsm(AppIo &io);
    void post_ready() { _ready = true; }
    void post_filters();
    void post_next();
    void post_render(bool ok);
    void post_service() { _service_pending = true; }

protected:
    int get_fail_state() const override;
    int handle_current_state() override;
    void on_enter_state(int state) override;

private:
    bool accepts_input() const;
    AppIo &_io;
    bool _ready = false;
    bool _next_pending = false;
    bool _filters_pending = false;
    bool _service_pending = false;
    bool _render_pending = false;
    bool _render_ok = false;
    bool _queued = false;
    static const Fsm::StateTransition _transitions[];
};
} // namespace cicala
