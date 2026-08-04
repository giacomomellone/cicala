/*
 * The tabletop state machine — the only thing in the firmware that decides
 * anything. Everything else reports or renders.
 *
 * The transition table in app_fsm.cpp is the specification; the diagram in
 * docs/firmware_architecture.md is the same table drawn. This class holds no
 * Zephyr headers and no channels: inputs are pushed in with post_*(), effects
 * go out through AppIo. That is what lets the suite drive a five-second
 * refresh timeout in microseconds, with no board and no display.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "fsm.hpp"

namespace tk
{

/** Everything the state machine can do to the world outside itself. */
class AppIo
{
public:
    virtual ~AppIo() = default;

    /**
     * Draw a question from `deck` and hand it to the display.
     *
     * @return false when the deck yields nothing — an empty corpus, or a
     *         filter that excludes everything in it.
     */
    virtual bool draw(uint8_t deck) = 0;

    /**
     * Put the deck's name on the panel.
     *
     * Turning the selector announces where it landed rather than answering
     * with a question nobody asked for. The name stays until Next is pressed,
     * which is what makes the deck legible without printing it on the case.
     *
     * @return false when the name could not be sent to the display.
     */
    virtual bool show_category(uint8_t deck) = 0;

    /**
     * True when the panel already holds a question drawn from `deck`.
     *
     * E-paper keeps its image without power, so the common wake is one where
     * there is nothing to do. This is what makes that case free.
     */
    virtual bool retained_matches(uint8_t deck) const = 0;
};

class AppFsm : public Fsm
{
public:
    enum class State { BOOT = 0, CATEGORY, SHOWING, DRAWING, REFRESHING, FAIL };

    /*
     * Named for what happened, not for where it goes — the table owns the
     * destinations. REDRAW, RELABEL and RETAINED exist because SHOWING and
     * BOOT have more than one way out and CONTINUE cannot mean all of them.
     */
    enum class Transition {
        REPEAT = 0, ///< nothing to do; stay put
        CONTINUE,   ///< the ordinary way forward
        REDRAW,     ///< Next: the table wants a question
        RELABEL,    ///< the selector moved: announce the new deck
        RETAINED,   ///< the panel already holds this deck's question
        FAILED,     ///< the draw or the render did not work
    };

    explicit AppFsm(AppIo &io);

    /** Latest selector reading. `valid` is false for zero or several contacts. */
    void post_selector(uint8_t deck, bool valid);

    /** One Next press happened. Dropped if it lands during a refresh. */
    void post_next();

    /** The display finished, successfully or not. */
    void post_render(bool ok);

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
    int on_fail();

    AppIo &_io;

    uint8_t _selector_deck = 0;
    bool _selector_valid = false;

    uint8_t _shown_deck = 0;

    bool _next_pending = false;
    bool _render_pending = false;
    bool _render_ok = false;
    bool _draw_ok = false;
    bool _label_ok = false;

    static const Fsm::StateTransition _transitions[];
};

} // namespace tk
