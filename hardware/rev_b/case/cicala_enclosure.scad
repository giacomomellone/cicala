// CERN-OHL-S-2.0. Rev B engineering prototype; dimensions from contract.json.
include <contract.scad>
include <pcb_component_bounds.scad>
part = "assembly";
stroke = 0;
print_rotation = [0,0,0];
print_shift = [0,0,0];
side_shift = [0,0,0];
lift = display_assembly_lift;
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
    difference() {
        translate(case_origin) round_box(case_width,case_depth,case_height);
        bevel_volume();
        block([case_control_ledge_x,-1,case_control_ledge_z],[20,case_depth+2,2]);
    }
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
module switch_body(index=0) {
    p=buttons_centres[index]; z=pcb_z+pcb_thickness;
    block([p.x-6,p.y-6,z],[12,12,3.5]);
    translate([p.x,p.y,z]) cylinder(d=7.1,h=5.5-stroke);
    block([p.x-1.9,p.y-1.9,z+5.5-stroke],[3.8,3.8,1.8]);
    for(x=[-6.25,6.25],y=[-2.5,2.5])
        block([p.x+x-.15,p.y+y-.5,z-buttons_lead_length],[.3,1,7]);
    for(y=[-4.5,4.5]) translate([p.x,p.y+y,z-1.5]) cylinder(d=1.6,h=1.5);
}
module switches() { for(i=[0:1]) switch_body(i); }
module rounded_block(p,size,r=.4) {
    translate(p) linear_extrude(size.z) offset(r=r)
        translate([r,r]) square([size.x-2*r,size.y-2*r]);
}
module cap(index=0,travel=stroke) {
    p=buttons_centres[index]; size=buttons_cap_sizes[index];
    z=pcb_z+pcb_thickness+buttons_assembled_height-buttons_cap_height-travel;
    // Purchased-part exterior/socket envelope. Internal ribs are not reproduced.
    translate(side_shift) difference() {
        rounded_block([p.x-size/2,p.y-size/2,z],[size,size,buttons_cap_height],.5);
        block([p.x-size/2+.8,p.y-size/2+.8,z-eps],[size-1.6,size-1.6,2.8+eps]);
    }
}
module caps() {
    color("#eee6cd") cap(0);
    color("#ed721f") cap(1);
}
module cap_slot(index) {
    p=buttons_centres[index]; size=buttons_cap_sizes[index]+2*buttons_radial_clearance;
    block([p.x-size/2,p.y-size/2,0],[size,size,case_height+5]);
}
module flex_reserve() { block([99.225,26.5,3.85+lift],[2.15,13,3.8]); }
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
    for (p=mounts) translate([p.x,p.y,5.75+lift]) cylinder(r=3.6,h=.6);
}
module top_shell() {
    difference() {
        intersection() {
            outer();
            union() {
                difference() {
                    outer();
                    block([case_origin.x+case_wall,case_wall,-1],
                          [case_width-2*case_wall,case_depth-2*case_wall,case_height-case_wall+1]);
                    block([case_origin.x-1,-1,-1],[case_width+2,case_depth+2,case_base+1]);
                }
                shell_posts(); frame_seats();
                // Recessed display bezel joins the roof and locates the lens.
                block([display_glass_origin.x-1.4,display_glass_origin.y-1.4,display_lens_z],
                      [display_glass.x+2.8,display_glass.y+2.8,case_height-display_lens_z]);
            }
        }
        flex_reserve(); glass(.2);
        block([display_glass_origin.x-.2,display_glass_origin.y-.2,display_lens_z-.15],
              [display_glass.x+.4,display_glass.y+.4,display_lens_thickness+.3]);
        block([display_active_origin.x-.5,display_active_origin.y-.5,display_lens_z-.25],
              [display_active.x+1,display_active.y+1,case_height]);
        port();
        for(i=[0:1]) cap_slot(i);
    }
}
module base() {
    difference() {
        union() {
            translate(case_origin) round_box(case_width,case_depth,case_base);
            for(p=mounts) translate([p.x,p.y,case_base]) {
                cylinder(r=2.3,h=pcb_z-case_base);
                cylinder(r1=3,r2=2.3,h=.8);
            }
        }
        screw_holes();
        for(p=mounts) translate([p.x,p.y,-eps]) cylinder(d1=4.8,d2=2.8,h=1.01);
    }
}
module display_frame_set() { translate([0,0,lift]) display_frame_set_local(); }
module display_frame_set_local() {
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
    difference() { display_frame_set(); block([30,58.2,6+lift],[43,4,2]); }
}
module display_support_bar() {
    intersection() { display_frame_set(); block([30,58.2,6+lift],[43,4,2]); }
}
module flex() { translate([0,0,lift]) flex_local(); }
module flex_local() {
    // Static fold former R1.55; the connector tip remains straight.
    translate([99.475,39.25,5.75]) rotate([90,0,0]) linear_extrude(12.5)
        difference() {
            circle(r=1.65); circle(r=1.55);
            translate([-2,-2]) square([2,4]);
        }
    block([99.075,26.75,7.3],[.4,12.5,.1]);
    block([94.9,26.75,4.1],[4.575,12.5,.1]);
}
module flex_former() { translate([0,0,lift]) flex_former_local(); }
module flex_former_local() {
    translate([99.475,39.25,5.75]) rotate([90,0,0]) linear_extrude(12.5)
        intersection() { circle(r=1.5); translate([0,-2]) square([2,4]); }
}
module harness() { translate([0,0,lift]) harness_local(); }
module harness_local() {
    // Reserved service loop beside the cell. Leads follow the edge of the
    // pouch; all joints use individual insulation and strain relief.
    block([64.7,30.5,3.7],[1.6,5,1.8]);
    block([62.8,29,3.7],[1.9,8,.9]);
}
module carrier_ring(outer=carrier_ring_outer_diameter, inner=carrier_ring_inner_diameter, height=1) {
    linear_extrude(height) difference() { circle(d=outer); circle(d=inner); }
}
module carrier_cover() {
    difference() {
        translate([carrier_ring_centre.x,carrier_ring_centre.y,-carrier_thickness])
            carrier_ring(carrier_ring_outer_diameter+1.4,carrier_ring_inner_diameter-1.4,carrier_cover_thickness);
        for(p=mounts) translate([p.x,p.y,-carrier_thickness-eps])
            cylinder(d1=5.2,d2=3.2,h=1.01);
    }
}
module carrier_screws() {
    for(p=mounts) translate([p.x,p.y,-carrier_thickness]) {
        cylinder(d1=4.8,d2=2.5,h=1);
        cylinder(d=2.5,h=12);
    }
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
            translate([case_origin.x-1.4,-1.4,-carrier_thickness]) round_box(case_width+2.8,case_depth+2.8,carrier_thickness,5.4);
            difference() {
                translate([case_origin.x-1.4,-1.4,-eps]) round_box(case_width+2.8,case_depth+2.8,2,5.4);
                translate([case_origin.x-.3,-.3,-1]) round_box(case_width+.6,case_depth+.6,4,4.3);
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
    translate([0,0,5*explode]) caps();
}

module selected_part() {
if(part=="assembly") assembly();
else if(part=="exploded") assembly(7);
else if(part=="section") intersection() { assembly(); block([case_origin.x,0,-3],[case_width,33,60]); }
else if(part=="inside") { color("#d0c6b6") base(); color("#32694e") pcb(); color("#aeb6ae") components(); color("#c6b66d") cell(); }
else if(part=="top_shell") top_shell();
else if(part=="base") base();
else if(part=="display_frame") display_frame();
else if(part=="display_support_bar") display_support_bar();
else if(part=="category_cap_reference") cap(0);
else if(part=="next_cap_reference") cap(1);
else if(part=="switch_reference") switch_body(0);
else if(part=="buttons_exploded") {
    color("#343731") switches();
    translate([0,0,5]) caps();
}
else if(part=="lens") lens();
else if(part=="carrier") carrier();
else if(part=="carrier_cover") carrier_cover();
else if(part=="carrier_assembly") { color("#d0c6b6") carrier();color("#e0d8c9") carrier_cover();color("#686f70") carrier_magnets();color("#8f9696") carrier_shield(); }
else if(part=="wedge") wedge();
else if(part=="flex_former") flex_former();
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
else if(part=="fit_caps_motion") intersection() {
    for(i=[0:1]) cap(i,stroke);
    union() {top_shell();display_frame_set();glass();lens();flex();components();cell();}
}
else if(part=="fit_caps_switch_body") intersection() {
    for(i=[0:1]) cap(i,stroke);
    for(p=buttons_centres) block([p.x-6,p.y-6,pcb_z+pcb_thickness],[12,12,3.5]);
}
else if(part=="fit_cap_insert") intersection() {
    translate([0,0,stroke]) for(i=[0:1]) cap(i); top_shell();
}
// Exclude the intended zero-clearance bearing plane on the base underside.
else if(part=="fit_carrier_fasteners") intersection() {carrier_screws();union(){carrier_cover();carrier_magnets();carrier_shield();}}
else if(part=="fit_carrier") intersection() { carrier(); union() { translate([0,0,.01]) base();top_shell(); } }
else assert(false,str("Unknown part: ",part));

}
translate(print_shift) rotate(print_rotation) selected_part();
