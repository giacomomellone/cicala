/** Where the retained block physically lives. */

#pragma once

#include "retained.hpp"

/** The block, already validated. */
kveld::Retained &kveld_retained();

/** True when this boot inherited a valid retained block. */
bool kveld_retained_survived();

/** Stamp the block so the next boot accepts it. */
void kveld_retained_seal();
