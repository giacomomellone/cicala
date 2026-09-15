// CERN-OHL-S-2.0. Rev B engineering prototype; dimensions from contract.json.
include <contract.scad>
include <pcb_component_bounds.scad>
part = "assembly";
stroke = 0;
coupon_clearance = buttons_radial_clearance;
keeper_adjustment = 0;
print_rotation = [0,0,0];
print_shift = [0,0,0];
side_shift = [0,0,0];
cap_relief = buttons_tip_relief;
coupon_relief = 0.1;
coupon_index = 0;
$fn = 48;
eps = 0.01;

module block(p, size) { translate(p) cube(size); }
module round_box(w,d,h,r=4) {
    linear_extrude(h) offset(r=r) translate([r,r]) square([w-2*r,d-2*r]);
}
module bevel_volume() {
    translate([case_bevel_start,case_depth+1,case_height]) rotate([90,0,0])
        linear_extrude(case_depth+2)
        polygon([[0,0],[20,-20*tan(case_bevel_angle)],[20,20],[0,20]]);
}
module outer() {
    difference() { round_box(case_width,case_depth,case_height); bevel_volume(); }
}
module glass(extra=0) {
    block(display_glass_origin-[extra,extra,extra],display_glass+[2*extra,2*extra,2*extra]);
}
module cell() { block(battery_reserved_origin,battery_reserved_size); }
module lens() {
    block([display_glass_origin.x,display_glass_origin.y,display_lens_z],
          [display_glass.x,display_glass.y,display_lens_thickness]);
}
module pcb() {
    translate([0,0,pcb_z]) linear_extrude(pcb_thickness) difference() {
        polygon(board_polygon);
        for (p=mounts) translate(p) circle(d=2.7);
    }
}
module components() {
    for (row=pcb_component_bounds) block(row[1],row[2]);
}
module switches() {
    for (p=buttons_centres) {
        block([p.x-3.1,p.y-3.1,pcb_z+pcb_thickness],[6.2,6.2,2.5]);
        translate([p.x,p.y,pcb_z+pcb_thickness]) cylinder(d=2,h=buttons_switch_height);
    }
}
module port() { block([71.7,60,pcb_z+.5],[12.6,10,4.7]); }
module screw_holes() {
    for (p=mounts) translate([p.x,p.y,-4]) cylinder(d=2.8,h=pcb_z+4+eps);
}
module shell_posts() {
    for (p=mounts) translate([p.x,p.y,pcb_z+pcb_thickness+.05]) difference() {
        cylinder(r=2.3,h=case_height-pcb_z-pcb_thickness-.6);
        translate([0,0,-eps]) cylinder(d=2.05,h=4.5);
    }
}
module frame_seats() {
    for (p=mounts) translate([p.x,p.y,5.75]) cylinder(r=3.6,h=.6);
}
keeper_top_z = buttons_retention_bottom_z-buttons_stop_travel;
keeper_bottom_z = keeper_top_z-buttons_keeper_thickness;
pad_bottom_z = pcb_z+pcb_thickness+.015+buttons_switch_height+buttons_tip_relief;
function keeper_points(index) = [for(sign=[-1,1])
    [buttons_keeper_screw_x,buttons_centres[index].y+sign*(buttons_cap_lengths[index]/2+buttons_keeper_screw_offset)]];
module rounded_block(p,size,r=.4) {
    translate(p) linear_extrude(size.z) offset(r=r)
        translate([r,r]) square([size.x-2*r,size.y-2*r]);
}
module cap_slot(index, clearance=buttons_radial_clearance) {
    p=buttons_centres[index]; len=buttons_cap_lengths[index];
    block([p.x-buttons_cap_width/2-clearance,p.y-len/2-clearance,-1],
          [buttons_cap_width+2*clearance,len+2*clearance,14]);
}
module retention_tabs(index, extra=0, bottom=buttons_retention_bottom_z, height=buttons_retention_thickness) {
    p=buttons_centres[index]; len=buttons_cap_lengths[index];
    for(sign=[-1,1]) {
        start=sign<0 ? p.y-len/2-buttons_retention_extension : p.y+len/2-buttons_retention_root_overlap;
        rounded_block([buttons_retention_x-extra,start-extra,bottom],
            [buttons_retention_width+2*extra,buttons_retention_extension+buttons_retention_root_overlap+2*extra,height],
            buttons_retention_radius);
    }
}
module tab_pockets(index,clearance=buttons_radial_clearance) {
    // Open underneath so the cap enters without bending either retaining tab.
    retention_tabs(index,clearance,-1,buttons_retention_bottom_z+buttons_retention_thickness+1+buttons_return_clearance);
}
module guide(index,clearance=buttons_radial_clearance) {
    p=buttons_centres[index]; len=buttons_cap_lengths[index];
    difference() {
        rounded_block([99.25,p.y-len/2-3,buttons_guide_bottom_z],[7.2,len+6,3],.6);
        cap_slot(index,clearance);tab_pockets(index,clearance);
        block([p.x-3.3,p.y-3.3,6],[6.6,6.6,1]);
    }
}
module keeper_posts(index) {
    for(p=keeper_points(index)) translate([p.x,p.y,keeper_top_z]) difference() {
        cylinder(r=2.2,h=4.2);
        translate([0,0,-eps]) cylinder(d=buttons_keeper_pilot_diameter,h=1.75+eps);
    }
}
module keeper_outline(index) {
    p=buttons_centres[index]; len=buttons_cap_lengths[index];
    translate([97.3,p.y-len/2-1.9]) square([1.7,len+3.8]);
    for(sign=[-1,1]) {
        y=p.y+sign*(len/2+.35);
        hull() {
            translate([98.15,y]) circle(r=.85);
            translate([101.1,y]) circle(r=1.45);
        }
        hull() {
            translate([101.1,y]) circle(r=1.45);
            translate([buttons_keeper_screw_x,p.y+sign*(len/2+buttons_keeper_screw_offset)]) circle(r=2.2);
        }
    }
}
module keeper(index=0, adjustment=keeper_adjustment) {
    difference() {
        union() {
            translate([0,0,keeper_bottom_z]) linear_extrude(buttons_keeper_thickness) keeper_outline(index);
            if(adjustment>0) retention_tabs(index,.1,keeper_top_z,adjustment);
        }
        if(adjustment<0) retention_tabs(index,.1,keeper_top_z+adjustment,-adjustment+eps);
        p=buttons_centres[index]; len=buttons_cap_lengths[index];
        for(sign=[-1,1]) {
            y=sign<0 ? p.y-len/2-.1-buttons_radial_clearance : p.y+len/2-buttons_strip_foot_inner_inset-buttons_radial_clearance;
            block([p.x-1.45-buttons_radial_clearance,y,keeper_bottom_z-eps],
                [2.9+2*buttons_radial_clearance,buttons_strip_foot_inner_inset+.1+2*buttons_radial_clearance,2.5]);
        }
        for(p=mounts) translate([p.x,p.y,5.55]) cylinder(r=3.85,h=2);
        for(p=keeper_points(index)) translate([p.x,p.y,keeper_bottom_z-eps]) {
            cylinder(d=2.2,h=buttons_keeper_thickness+eps*2);
            cylinder(d1=buttons_keeper_head_diameter,d2=2.2,h=1.0+eps);
        }
    }
}
module keeper_screws(index=0) {
    for(p=keeper_points(index)) translate([p.x,p.y,keeper_bottom_z]) {
        cylinder(d1=4,d2=2,h=1);
        cylinder(d=2,h=buttons_keeper_screw_length);
    }
}
module cap(index=0, travel=stroke, relief=cap_relief, flat=false) {
    p=buttons_centres[index]; len=buttons_cap_lengths[index]; x=p.x-buttons_cap_width/2;
    contact_z=pcb_z+pcb_thickness+.015+buttons_switch_height+relief;
    translate(side_shift+[0,0,-travel]) union() {
      difference() {
        union() {
            translate([x,p.y+len/2,0]) rotate([90,0,0]) linear_extrude(len)
                polygon(flat ? [[0,buttons_cap_body_bottom_z],[buttons_cap_width,buttons_cap_body_bottom_z],
                                [buttons_cap_width,8.35],[0,8.35]] :
                    [[0,buttons_cap_body_bottom_z],[buttons_cap_width,buttons_cap_body_bottom_z],
                     [buttons_cap_width,case_height+.1-(x+buttons_cap_width-case_bevel_start)*tan(case_bevel_angle)],
                     [case_bevel_start-x+(.1+buttons_face_recess)/tan(case_bevel_angle),case_height-buttons_face_recess],
                     [0,case_height-buttons_face_recess]]);
            retention_tabs(index);
            for(sign=[-1,1]) {
                y=sign<0 ? p.y-len/2 : p.y+len/2-buttons_strip_foot_inner_inset;
                rounded_block([p.x-1.45,y,min(buttons_strip_foot_bottom_z,contact_z-.85)],
                    [2.9,buttons_strip_foot_inner_inset,buttons_cap_body_top_overlap-min(buttons_strip_foot_bottom_z,contact_z-.85)],.3);
            }
        }
        block([p.x-buttons_pad_width/2-buttons_pad_slot_clearance,p.y-len/2+buttons_strip_foot_inner_inset,5],
              [buttons_pad_width+2*buttons_pad_slot_clearance,len-2*buttons_strip_foot_inner_inset,
               contact_z+buttons_compliant_pad_thickness+.15-5]);
        for(sign=[-1,1]) {
            y=sign<0 ? p.y-len/2+buttons_pad_end_inset-buttons_pad_slot_clearance : p.y+len/2-buttons_strip_foot_inner_inset-eps;
            block([p.x-buttons_pad_width/2-buttons_pad_slot_clearance,y,contact_z-.025],
                [buttons_pad_width+2*buttons_pad_slot_clearance,buttons_strip_foot_inner_inset-buttons_pad_end_inset+buttons_pad_slot_clearance+eps,
                 buttons_compliant_pad_thickness+.05]);
        }
      }
      translate([p.x,p.y,contact_z+buttons_compliant_pad_thickness])
        cylinder(d=buttons_tip_diameter,h=max(buttons_cap_body_bottom_z+.1,contact_z+buttons_compliant_pad_thickness+.25)-contact_z-buttons_compliant_pad_thickness);
    }
}
module cap_pad(index=0,travel=stroke,relief=cap_relief) {
    p=buttons_centres[index]; len=buttons_cap_lengths[index];
    block([p.x-buttons_pad_width/2,p.y-len/2+buttons_pad_end_inset,
           pcb_z+pcb_thickness+.015+buttons_switch_height+relief-travel],
          [buttons_pad_width,len-2*buttons_pad_end_inset,buttons_compliant_pad_thickness]);
}
module top_shell() {
    difference() {
        intersection() {
            outer();
            union() {
                difference() {
                    outer();
                    difference() {
                        block([case_wall,case_wall,-1],[case_width-2*case_wall,case_depth-2*case_wall,9.55]);
                        translate([0,0,-1.2]) bevel_volume();
                    }
                    block([-1,-1,-1],[case_width+2,case_depth+2,2.2]);
                }
                shell_posts(); frame_seats();
                for(i=[0:1]) { guide(i);keeper_posts(i); }
            }
        }
        glass(.2);
        block([display_glass_origin.x-.2,display_glass_origin.y-.2,8.5],
              [display_glass.x+.4,display_glass.y+.4,1.1]);
        block([display_active_origin.x-.5,display_active_origin.y-.5,8.4],
              [display_active.x+1,display_active.y+1,3]);
        port();
        for(i=[0:1]) { cap_slot(i); tab_pockets(i); }
    }
}
module base() {
    difference() {
        union() {
            round_box(case_width,case_depth,case_base);
            for(p=mounts) translate([p.x,p.y,case_base]) {
                cylinder(r=2.3,h=pcb_z-case_base);
                cylinder(r1=3,r2=2.3,h=.8);
            }
        }
        screw_holes();
        for(p=mounts) translate([p.x,p.y,-eps]) cylinder(d1=4.8,d2=2.8,h=1.01);
    }
}
module display_frame_set() {
    difference() {
        block([13.375,4.795,6.35],[86.425,56.41,.8]);
        block([15.745,7.165,6.2],[76.51,51.17,1.1]);
        block([94.8,26.15,6.1],[7,13.7,1.3]);
        block([72.8,58.2,6.1],[10.4,4.2,1.3]);
        // The module occupies the lower left; the frame supports the other
        // three perimeter regions without placing a load on its RF shield.
        block([12,43.5,6.1],[18.5,20,1.3]);
        for(p=mounts) translate([p.x,p.y,6.1]) cylinder(r=2.55,h=1.2);
    }
}
module display_frame() {
    difference() { display_frame_set(); block([30,58.2,6],[43,4,2]); }
}
module display_support_bar() {
    intersection() { display_frame_set(); block([30,58.2,6],[43,4,2]); }
}
module flex() {
    // Static fold former R1.55; the connector tip remains straight.
    translate([99.475,39.25,5.75]) rotate([90,0,0]) linear_extrude(12.5)
        difference() {
            circle(r=1.65); circle(r=1.55);
            translate([-2,-2]) square([2,4]);
        }
    block([99.075,26.75,7.3],[.4,12.5,.1]);
    block([94.9,26.75,4.1],[4.575,12.5,.1]);
}
module flex_former() {
    translate([99.475,39.25,5.75]) rotate([90,0,0]) linear_extrude(12.5)
        intersection() { circle(r=1.5); translate([0,-2]) square([2,4]); }
}
module harness() {
    // Reserved service loop beside the cell. Leads follow the edge of the
    // pouch; all joints use individual insulation and strain relief.
    block([64.7,30.5,3.7],[1.6,5,1.8]);
    block([62.8,29,3.7],[1.9,8,.9]);
}
module carrier_ring(outer=carrier_ring_outer_diameter, inner=carrier_ring_inner_diameter, height=1) {
    linear_extrude(height) difference() { circle(d=outer); circle(d=inner); }
}
module carrier_cover() {
    translate([carrier_ring_centre.x,carrier_ring_centre.y,-carrier_thickness])
        carrier_ring(carrier_ring_outer_diameter+1.4,carrier_ring_inner_diameter-1.4,carrier_cover_thickness);
}
module carrier_magnets() {
    translate([carrier_ring_centre.x,carrier_ring_centre.y,-carrier_thickness+carrier_cover_thickness])
        carrier_ring(height=carrier_magnet_thickness);
}
module carrier_shield() {
    translate([carrier_ring_centre.x,carrier_ring_centre.y,-carrier_support_thickness-carrier_steel_shield_thickness]) carrier_ring(height=carrier_steel_shield_thickness);
}
module carrier() {
    difference() {
        union() {
            translate([-1.4,-1.4,-carrier_thickness]) round_box(case_width+2.8,case_depth+2.8,carrier_thickness,5.4);
            difference() {
                translate([-1.4,-1.4,-eps]) round_box(case_width+2.8,case_depth+2.8,2,5.4);
                translate([-.3,-.3,-1]) round_box(case_width+.6,case_depth+.6,4,4.3);
            }
        }
        translate([carrier_ring_centre.x,carrier_ring_centre.y,-carrier_thickness-eps])
            carrier_ring(carrier_ring_outer_diameter+.4,carrier_ring_inner_diameter-.4,2.7+eps);
        translate([carrier_ring_centre.x,carrier_ring_centre.y,-carrier_thickness-eps])
            carrier_ring(carrier_ring_outer_diameter+1.6,carrier_ring_inner_diameter-1.6,.85+eps);
        screw_holes(); port();
        for(p=mounts) translate([p.x,p.y,-carrier_thickness-eps]) cylinder(d1=4.8,d2=2.8,h=1.01);
        block([70,64,-1],[16,5,4]);
    }
}
module wedge() {
    // Passive 12-degree stand. Print on either triangular side.
    rotate([90,0,90]) linear_extrude(80)
        polygon([[0,0],[36,0],[36,10],[0,2.35]]);
}
module coupon(index=0) {
    p=buttons_centres[index];
    intersection() {
        difference() {
            union() {
                guide(index,coupon_clearance);keeper_posts(index);
                block([99.25,p.y-12,8.0],[7.35,24,1.2]);
                block([106,p.y-12,5.9],[.6,24,3.3]);
            }
            cap_slot(index,coupon_clearance);tab_pockets(index,coupon_clearance);
        }
        outer();
    }
}
module assembly(explode=0) {
    color("#d0c6b6") base();
    color("#c6b66d") cell();
    color("#32694e") pcb();
    color("#aeb6ae") components(); color("#ca9b49") switches();
    color("#dba960") flex(); color("#a34e3c") harness();
    translate([0,0,explode]) color("#998e77") { display_frame();display_support_bar(); }
    translate([0,0,2*explode]) color("#b8b8a7") glass();
    translate([0,0,3*explode]) color([.8,.85,.8,.25]) lens();
    translate([0,0,4*explode]) color("#e0d8c9") top_shell();
    translate([0,0,4*explode]) color("#c26b4a") for(i=[0:1]) cap(i);
    translate([0,0,4*explode]) {
        color("#7b7d72") for(i=[0:1]) cap_pad(i);
        color("#8f9989") for(i=[0:1]) keeper(i);
        color("#a8aaa5") for(i=[0:1]) keeper_screws(i);
    }
}

module selected_part() {
if(part=="assembly") assembly();
else if(part=="exploded") assembly(7);
else if(part=="section") intersection() { assembly(); block([0,0,-3],[110,33,60]); }
else if(part=="inside") { color("#d0c6b6") base(); color("#32694e") pcb(); color("#aeb6ae") components(); color("#c6b66d") cell(); }
else if(part=="top_shell") top_shell();
else if(part=="base") base();
else if(part=="display_frame") display_frame();
else if(part=="display_support_bar") display_support_bar();
else if(part=="category_cap") cap(0);
else if(part=="next_cap") cap(1);
else if(part=="category_keeper") keeper(0);
else if(part=="next_keeper") keeper(1);
else if(part=="keeper_coupon") keeper(coupon_index);
else if(part=="buttons") { color("#c26b4a") cap(0);color("#8f9989") keeper(0);color("#7b7d72") cap_pad(0); }
else if(part=="buttons_exploded") { color("#c26b4a") translate([0,0,4]) cap(0);color("#8f9989") keeper(0);color("#7b7d72") translate([0,0,2]) cap_pad(0);color("#a8aaa5") translate([0,0,-3]) keeper_screws(0); }
else if(part=="flat_cap") cap(0,0,buttons_tip_relief,true);
else if(part=="lens") lens();
else if(part=="carrier") carrier();
else if(part=="carrier_cover") carrier_cover();
else if(part=="carrier_assembly") { color("#d0c6b6") carrier();color("#e0d8c9") carrier_cover();color("#686f70") carrier_magnets();color("#8f9696") carrier_shield(); }
else if(part=="wedge") wedge();
else if(part=="coupon") coupon(coupon_index);
else if(part=="flex_former") flex_former();
else if(part=="cap_pad") cap_pad(0);
else if(part=="next_pad") cap_pad(1);
else if(part=="coupon_cap") cap(coupon_index,0,coupon_relief);
else if(part=="pcb_reference") pcb();
else if(part=="cell_reference") cell();
else if(part=="panel_reference") glass();
else if(part=="fit_components_case") intersection() { components(); union() { top_shell(); base(); display_frame_set(); } }
else if(part=="fit_components_cell") intersection() { components(); cell(); }
else if(part=="fit_switches_case") intersection() { switches(); union() { top_shell(); base(); display_frame_set(); glass(); } }
else if(part=="fit_pcb_cell") intersection() { pcb(); cell(); }
else if(part=="fit_panel_case") intersection() { glass(); union() { top_shell();base();display_frame_set(); } }
else if(part=="fit_flex_case") intersection() { flex(); union() { top_shell();display_frame_set(); } }
else if(part=="fit_flex_components") intersection() { flex(); for(row=pcb_component_bounds) if(row[0]!="J2") block(row[1],row[2]); }
else if(part=="fit_harness") intersection() { harness(); union() { components();top_shell();base(); } }
else if(part=="fit_caps_motion") intersection() { for(i=[0:1]) cap(i,stroke); union() { top_shell();display_frame_set();glass();components();cell();for(i=[0:1]) keeper(i); } }
else if(part=="fit_caps_switch_body") intersection() { for(i=[0:1]) cap(i,stroke);for(p=buttons_centres) block([p.x-3.1,p.y-3.1,pcb_z+pcb_thickness],[6.2,6.2,2.5]); }
else if(part=="fit_keepers") intersection() { for(i=[0:1]) keeper(i); union() { top_shell();components();switches();glass();flex();display_frame_set();pcb(); } }
else if(part=="fit_cap_insert") intersection() { translate([0,0,-stroke]) for(i=[0:1]) cap(i);top_shell(); }
else if(part=="fit_keeper_insert") intersection() { translate([0,0,-stroke]) for(i=[0:1]) keeper(i);union() { top_shell();for(i=[0:1]) cap(i); } }
else if(part=="fit_caps_lateral") intersection() { for(i=[0:1]) cap(i,stroke); union() {top_shell();display_frame_set();glass();components();for(i=[0:1]) keeper(i);} }
else if(part=="fit_strip") intersection() { for(i=[0:1]) cap_pad(i); union() { for(i=[0:1]) cap(i);top_shell();components(); } }
else if(part=="fit_return_contact") intersection() { for(i=[0:1]) cap(i,-buttons_return_clearance-.02);top_shell(); }
else if(part=="fit_screws") intersection() {for(i=[0:1]) keeper_screws(i);union(){components();switches();glass();flex();pcb();}}
else if(part=="fit_stop_contact") intersection() { for(i=[0:1]) cap(i,buttons_stop_travel-keeper_adjustment+.02); for(i=[0:1]) keeper(i); }
// Exclude the intended zero-clearance bearing plane on the base underside.
else if(part=="fit_carrier") intersection() { carrier(); union() { translate([0,0,.01]) base();top_shell(); } }
else assert(false,str("Unknown part: ",part));

}
translate(print_shift) rotate(print_rotation) selected_part();
