/* Power states shared by the power and status modules. */

#pragma once

namespace cicala
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
    /** Charging, or external power with the breadboard's voltage estimate. */
    CHARGING,
    /** Breadboard voltage estimate; status pins cannot prove charge completion. */
    CHARGED,
    /** External power, with charging idle, disabled, or status unavailable. */
    EXTERNAL_IDLE,
    /** The charger reports a recoverable or latched fault. */
    CHARGE_FAULT,
};

enum class ChargerStatus {
    NOT_MONITORED = 0,
    CHARGING,
    IDLE,
    RECOVERABLE_FAULT,
    LATCHED_FAULT,
    UNAVAILABLE,
};

} // namespace cicala
