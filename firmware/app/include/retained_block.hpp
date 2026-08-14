/** Where the retained block physically lives. */

#pragma once

#include "retained.hpp"

/** The block, already validated. */
tk::Retained &tk_retained();

/** True when this boot inherited a valid retained block. */
bool tk_retained_survived();

/** Stamp the block so the next boot accepts it. */
void tk_retained_seal();
