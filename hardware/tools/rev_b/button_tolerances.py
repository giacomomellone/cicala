"""Screen recessed cap access and lead clearance using drawing tolerances."""
import math


def audit(contract):
    b = contract['buttons']; c = contract['case']; pcb = contract['pcb']
    radial = b['radial_clearance'] - (b['cap_size_tolerance'] + b['printed_aperture_tolerance']) / 2 - b['alignment_tolerance']
    cap_top = pcb['z'] + pcb['thickness'] + b['assembled_height']
    recess = c['control_ledge_z'] - cap_top
    vertical = b['assembled_height_tolerance'] + b['pcb_thickness_tolerance'] + b['vertical_print_tolerance'] + b['pcb_seat_height_tolerance']
    minimum_recess = recess - vertical
    maximum_pressed = recess + vertical + b['switch_travel_range'][1]
    lead = pcb['z'] + pcb['thickness'] - b['pcb_thickness_tolerance'] - b['lead_length'] - b['lead_length_tolerance'] - c['base'] - b['vertical_print_tolerance'] - b['pcb_seat_height_tolerance']
    if radial <= 0:
        raise ValueError('Cap can bind in the printed aperture')
    if abs(recess - b['nominal_recess']) > 1e-6 or minimum_recess <= 0:
        raise ValueError('Cap can protrude above the surrounding panel')
    if maximum_pressed > b['maximum_pressed_recess']:
        raise ValueError('Pressed cap is too deeply recessed for the access well')
    if lead <= 0:
        raise ValueError('Through-hole lead can touch the base')
    joint = pcb['z'] - b['pcb_seat_height_tolerance'] - c['base'] - b['vertical_print_tolerance'] - b['max_solder_projection']
    if joint <= 0:
        raise ValueError('Specified solder envelope can touch the base')
    # A rigid spherical probe is a reproducible access check, not a finger model.
    probe_margin = float('inf')
    radius = b['access_probe_radius']
    for i in range(501):
        depth = maximum_pressed * i / 500
        probe_half = math.sqrt(max(0, 2 * radius * depth - depth * depth))
        below_roof = maximum_pressed - depth
        flare = b['well_flare'] * max(0, 1 - below_roof / b['well_depth'])
        opening_half = min(b['cap_sizes']) / 2 + b['radial_clearance'] - b['printed_aperture_tolerance'] / 2 - b['alignment_tolerance'] + flare
        probe_margin = min(probe_margin, opening_half - probe_half)
    if probe_margin <= 0:
        raise ValueError('Access probe hits the well before maximum pretravel')
    return {
        'kind': 'Geometric screening using Omron drawing tolerances and declared print/assembly assumptions',
        'nominal_recess_mm': round(recess, 3),
        'minimum_unpressed_recess_mm': round(minimum_recess, 3),
        'maximum_pressed_recess_mm': round(maximum_pressed, 3),
        'minimum_radial_clearance_mm': round(radial, 3),
        'minimum_access_probe_clearance_mm': round(probe_margin, 3),
        'access_probe_radius_mm': radius,
        'minimum_lead_base_clearance_mm': round(lead, 3),
        'minimum_specified_solder_base_clearance_mm': round(joint, 3),
        'nominal_cap_top_z_mm': cap_top,
        'maximum_specified_pretravel_mm': b['switch_travel_range'][1],
        'qualification': 'Check cap seating, full return, off-centre actuation and finished solder-joint height on the first article. Probe access does not qualify finger comfort. Pretravel is not an allowable overtravel or overload specification.',
    }
