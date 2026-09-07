"""Bound the charge veto using the manufacturer's thermistor R/T table.

Sources and design allowances are documented in ../REVIEW.md. This checks a
static tolerance model; it does not replace thermal or USB insertion testing.
"""
import itertools
import json
import math
import sys

from ksexp import child, children, load

# Semitec 103AT-2, published AT-series table, resistance in ohms.
RT = ((0, 27280), (10, 17960), (20, 12090), (25, 10000),
      (30, 8313), (40, 5827), (50, 4160), (60, 3020))


def resistance(temp):
    if not RT[0][0] <= temp <= RT[-1][0]:
        raise ValueError("Temperature is outside the published table interval")
    for (t0, r0), (t1, r1) in zip(RT, RT[1:]):
        if t0 <= temp <= t1:
            fraction = (1 / (temp + 273.15) - 1 / (t0 + 273.15)) / (
                1 / (t1 + 273.15) - 1 / (t0 + 273.15))
            return math.exp(math.log(r0) + fraction * math.log(r1 / r0))
    raise AssertionError("Missing table interval")


def thresholds(r35=820000, r36=210000, r37=82500):
    if min(r35, r36, r37) <= 0:
        raise ValueError("Reference resistances must be positive")
    cold, hot = [], []
    for supply, a, b, c, offset in itertools.product(
            (3.135, 3.465), (.99, 1.01), (.99, 1.01), (.99, 1.01), (-.005, .005)):
        total = r35 * a + r36 * b + r37 * c
        cold.append(supply * (r36 * b + r37 * c) / total + offset)
        hot.append(supply * r37 * c / total + offset)
    return (min(cold), max(cold)), (min(hot), max(hot))


def verify(r35=820000, r36=210000, r37=82500):
    cold, hot = thresholds(r35, r36, r37)
    # +/-5% is a conservative design allowance for R/T tolerance and assembly
    # effects. The bonded sensor's dynamic lag remains a physical measurement.
    ntc_min = lambda t: resistance(t) * .95 * 36.5e-6
    ntc_max = lambda t: resistance(t) * 1.05 * 39.5e-6
    margins = {
        "cold_stop_at_0C_V": ntc_min(0) - cold[1],
        "hot_stop_at_45C_V": hot[0] - ntc_max(45),
        "permission_at_10C_V": cold[0] - ntc_max(10),
        "permission_at_30C_V": ntc_min(30) - hot[1],
    }
    if min(margins.values()) <= 0:
        raise ValueError(f"Charge temperature tolerance budget failed: {margins}")
    total = r35 + r36 + r37
    nominal_cold = 3.3 * (r36 + r37) / total
    nominal_hot = 3.3 * r37 / total
    allowed = [i / 100 for i in range(6001)
               if nominal_hot < resistance(i / 100) * 38e-6 < nominal_cold]
    return {"cold_threshold_V": cold, "hot_threshold_V": hot,
            "nominal_charge_window_C": [allowed[0], allowed[-1]],
            "reference_current_uA": 3.3 / total * 1e6,
            "model_margins_V": margins}


def ohms(value):
    return float(value.rstrip('kM')) * (1e6 if value.endswith('M') else
                                       1e3 if value.endswith('k') else 1)


def audit(netlist):
    data = load(netlist)
    values = {child(c, 'ref')[1]: child(c, 'value')[1]
              for c in children(child(data, 'components'), 'comp')}
    result = verify(*(ohms(values[r]) for r in ('R35', 'R36', 'R37')))
    ce_low = 5.5 * ohms(values['R13']) * 1.01 / (
        ohms(values['R39']) * .99 + ohms(values['R13']) * 1.01)
    if ce_low >= .4:
        raise ValueError(f"Charger cannot enable with GPIO floating: CE={ce_low:.3f} V")
    result['floating_gpio_CE_max_V'] = ce_low
    return result


if __name__ == '__main__':
    print(json.dumps(audit(sys.argv[1]), indent=2))
