"""Check purchased-cap aperture and lead clearances against drawing tolerances."""

def audit(contract):
    b=contract['buttons']; c=contract['case']; pcb=contract['pcb']
    radial=b['radial_clearance']-(b['cap_size_tolerance']+b['printed_aperture_tolerance'])/2-b['alignment_tolerance']
    pressed=pcb['z']+pcb['thickness']+b['assembled_height']-b['assembled_height_tolerance']-b['pcb_thickness_tolerance']-b['switch_travel_range'][1]-c['control_ledge_z']-b['vertical_print_tolerance']-b['pcb_seat_height_tolerance']
    lead=pcb['z']+pcb['thickness']-b['pcb_thickness_tolerance']-b['lead_length']-b['lead_length_tolerance']-c['base']-b['vertical_print_tolerance']-b['pcb_seat_height_tolerance']
    if radial <= 0:raise ValueError('Cap can bind in the printed aperture')
    if pressed <= 0:raise ValueError('Cap can sink below the panel before maximum specified pretravel')
    if lead <= 0:raise ValueError('Through-hole lead can touch the base')
    return {'kind':'Geometric screening using Omron drawing tolerances and declared print/assembly assumptions',
            'minimum_radial_clearance_mm':round(radial,3),'minimum_pressed_cap_proud_mm':round(pressed,3),
            'minimum_lead_base_clearance_mm':round(lead,3),'nominal_cap_top_z_mm':pcb['z']+pcb['thickness']+b['assembled_height'],
            'maximum_specified_pretravel_mm':b['switch_travel_range'][1],
            'qualification':'Check cap seating, full return, off-centre actuation and finished solder-joint height on the first article. Pretravel is not an allowable overtravel or overload specification.'}
