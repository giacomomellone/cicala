/* Power states shared by the power and status modules. */

#pragma once

namespace kveld
{

enum class PowerState {
    /** No plausible measurement. Refreshes remain allowed. */
    UNKNOWN = 0,
    /** On the cell, above the refresh floor. */
    NORMAL,
    /** On the cell, under the refresh floor. Refreshes are refused. */
    LOW,
    /** On the cell, nearly flat. Refreshes are refused. */
    CRITICAL,
    /** External power is in. Says nothing about whether current is flowing. */
    CHARGING,
    /** External power is present and the cell reads above the full estimate. */
    CHARGED,
};

} // namespace kveld
