/*
 * Where the retained block physically lives.
 *
 * The block itself, and the rules for deciding whether it survived, are
 * lib/retained. This is the one file that knows it belongs in RTC slow memory
 * rather than in `.bss`, which is the whole difference between state that
 * survives deep sleep and state that does not.
 *
 * `.hpp` rather than `.h` like its neighbours because it hands out a C++ type;
 * both callers, app_logic.cpp and panel.cpp, are C++ already.
 */

#pragma once

#include "retained.hpp"

/**
 * The block, already validated.
 *
 * Zeroed on the first call of a boot if it did not survive, so a caller never
 * sees garbage. `tk_retained_survived()` is how to tell which happened.
 */
tk::Retained &tk_retained();

/**
 * True when this boot inherited a valid block — a wake rather than a cold
 * power-on.
 *
 * Calling this before `tk_retained()` returns the same answer: the check runs
 * once, at first use, whichever function asks for it.
 */
bool tk_retained_survived();

/**
 * Stamp the block so the next boot accepts it.
 *
 * Call after the state has settled, not on every field write. An unsealed
 * block is discarded on the next boot, so the cost of sealing too rarely is a
 * forgotten cycle of the shuffle bag, and the cost of not sealing at all would
 * be silently retaining nothing.
 */
void tk_retained_seal();
