"""Screen measured cap/keeper selection; this is displacement, not a force model."""
from itertools import product


def select(reliefs, adjustments, nominal_stop, stack_error, stop_error, switch_travel,
           gap_range, compression_range):
    choices = []
    for relief, adjustment in product(reliefs, adjustments):
        gap = relief - stack_error
        stop = nominal_stop - adjustment + stop_error
        compression = stop - gap - switch_travel
        if (gap_range[0] - 1e-9 <= gap <= gap_range[1] + 1e-9
                and compression_range[0] - 1e-9 <= compression <= compression_range[1] + 1e-9):
            choices.append((abs(gap - sum(gap_range)/2) + abs(compression - sum(compression_range)/2),
                            relief, adjustment, gap, stop, compression))
    if not choices:
        return None
    _, relief, adjustment, gap, stop, compression = min(choices)
    return dict(relief=relief, keeper_adjustment=adjustment, rest_gap=round(gap,6),
                stop=round(stop,6), pad_compression=round(compression,6))


def audit(contract):
    b = contract['buttons']; a = b['tolerance_assumptions']
    terms = [a[k] for k in ('pcb_thickness_plus_minus','switch_height_plus_minus',
                            'printed_vertical_plus_minus','solder_height_plus_minus',
                            'silicone_thickness_plus_minus')]
    total = sum(terms)
    cases = []
    for signs in product((-1,1), repeat=len(terms)):
        error = sum(sign*value for sign,value in zip(signs,terms))
        for stop_error, travel in product((-a['printed_vertical_plus_minus'],a['printed_vertical_plus_minus']),
                                         (a['switch_travel_range'][0],b['switch_travel'],a['switch_travel_range'][1])):
            choice = select(b['coupon_reliefs'],b['keeper_stop_adjustments'],b['stop_travel'],
                            error,stop_error,travel,a['measured_gap_target'],a['pad_compression_range'])
            cases.append(dict(stack_error=round(error,6),stop_error=stop_error,switch_travel=travel,selection=choice))
    engagement = b['retention_extension'] - 2*b['radial_clearance']
    if engagement < b['minimum_tab_engagement']:
        raise ValueError('Retaining tab can lose its required overlap at full lateral play')
    if b['retention_thickness'] < 1.2:
        raise ValueError('Retaining tab cross section is too thin')
    if any(case['selection'] is None for case in cases):
        raise ValueError('Graded cap/keeper set does not cover the declared displacement corners')
    return {
        'kind':'geometric tolerance screening; no silicone compression-force or life prediction',
        'assumptions':a,
        'fixed_nominal_cap_gap_range':[round(b['tip_relief']-total,6),round(b['tip_relief']+total,6)],
        'fixed_parts_guarantee_actuation':False,
        'minimum_retaining_overlap_at_full_play':round(engagement,6),
        'retaining_tab_thickness':b['retention_thickness'],
        'screened_corner_combinations':len(cases),
        'all_corners_have_a_measured_selection':True,
        'cases':cases,
        'qualification':'Measure actual rest gap, electrical actuation, stop position and force on the coupon. Select each cap/keeper pair before fitting the display. Published switch travel and assumed pad displacement do not prove assembled operation.'}
