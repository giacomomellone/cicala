/* Tabletop state machine. Inputs arrive through post_*(); effects use AppIo. */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "fsm.hpp"

namespace kveld
{

/** Effects performed by the tabletop state machine. */
class AppIo
{
public:
    virtual ~AppIo() = default;

    /** Draw from `deck`; return false if no question is available. */
    virtual bool draw(uint8_t deck) = 0;

    /** Show the deck name; return false if it could not be queued. */
    virtual bool show_category(uint8_t deck) = 0;

    /** Show the current service card; return false if it could not be queued. */
    virtual bool show_service() = 0;

    /** Return whether the retained panel shows a question from `deck`. */
    virtual bool retained_matches(uint8_t deck) const = 0;
};

class AppFsm : public Fsm
{
public:
    enum class State { BOOT = 0, CATEGORY, SHOWING, DRAWING, REFRESHING, SERVICE, FAIL };

    enum class Transition {
        REPEAT = 0, ///< nothing to do; stay put
        CONTINUE,   ///< the ordinary way forward
        REDRAW,     ///< Next: the table wants a question
        RELABEL,    ///< the active category changed
        RETAINED,   ///< the panel already holds this deck's question
        SERVICE,    ///< the setup portal has something to say
        FAILED,     ///< the draw or the render did not work
    };

    explicit AppFsm(AppIo &io);

    /** Post the active deck and whether the value is usable. */
    void post_selector(uint8_t deck, bool valid);

    /** Post one Next press. Presses during a refresh are dropped. */
    void post_next();

    /** Post the result of a display refresh. */
    void post_render(bool ok);

    /** Request the setup portal's current service card. */
    void post_service();

    /** The deck the panel is currently showing. */
    uint8_t shown_deck() const { return _shown_deck; }

protected:
    int get_fail_state() const override;
    int handle_current_state() override;
    void on_enter_state(int state) override;

private:
    int on_boot();
    int on_category();
    int on_showing();
    int on_drawing();
    int on_refreshing();
    int on_service();
    int on_fail();

    AppIo &_io;

    uint8_t _selector_deck = 0;
    bool _selector_valid = false;

    uint8_t _shown_deck = 0;

    bool _next_pending = false;
    bool _service_pending = false;
    bool _service_ok = false;
    bool _render_pending = false;
    bool _render_ok = false;
    bool _draw_ok = false;
    bool _label_ok = false;

    static const Fsm::StateTransition _transitions[];
};

} // namespace kveld
