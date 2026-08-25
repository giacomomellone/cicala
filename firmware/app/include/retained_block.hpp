/** Where the retained block physically lives. */

#pragma once

#include "retained.hpp"

/** The block, already validated. */
cicala::Retained &cicala_retained();

/** True when this boot inherited a valid retained block. */
bool cicala_retained_survived();

/** Stamp the block so the next boot accepts it. */
void cicala_retained_seal();
