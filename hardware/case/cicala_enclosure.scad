/*
 * Cicala enclosure, Rev A mechanical contract
 *
 * Canonical coordinates are millimetres. X runs left to right, Y runs from
 * rear to front, and Z runs upward from the table. The model is deliberately
 * parametric and conservative: fit-critical details remain coupon-gated.
 *
 * SPDX-License-Identifier: CERN-OHL-S-2.0
 */

$fn = $preview ? 48 : 96;

part = "assembly"; // [assembly,exploded,section,top_shell,base,retainer,category_cap,next_cap,lens,steel_skin,light_pipe,pcb_reference,coupon_buttons,coupon_buttons_assembly,coupon_usb,coupon_lens,coupon_boss]
section_axis = "x"; // [x,y,z]
section_position_percent = 50; // [0:1:100]
section_keep = "positive"; // [negative,positive]

// Public Cicala finish intent. Physical colour and texture remain sample-gated.
paper_color = [250 / 255, 248 / 255, 242 / 255];
paper_raised_color = [1, 1, 254 / 255];
ink_color = [31 / 255, 31 / 255, 29 / 255];
ink_soft_color = [95 / 255, 94 / 255, 90 / 255];

// Product envelope.
case_width = 84;
case_depth = 56;
corner_radius = 3;
face_rear_z = 15.85;
face_front_z = 12.91;
face_angle = atan(
    (face_rear_z - face_front_z) /
    (case_depth - 2 * corner_radius)
);

// Stack from the table upward.
foot_height = 1.5;
steel_thickness = 1.2;
base_thickness = 1.2;
steel_z = foot_height;
base_z = steel_z + steel_thickness;
seam_z = base_z + base_thickness;

// Shell construction. Do not tune fits before printing the coupons.
wall = 2.0;
roof = 2.0;
fit_clearance_xy = 0.25;
boss_outer_diameter = 6.0;
boss_pilot_diameter = 2.1;      // M2.5 thread-forming screw from below
boss_screw_diameter = 2.5;
pillar_diameter = 5.0;          // roof pillar that holds the board down
mount_hole_diameter = 2.7;
cell_recess_depth = 0.35;

// PCB datum: rear-left corner at (3, 3). Everything except the two switches
// and the display stack is on the underside, in the cell cavity, because the
// sloped roof leaves only 0.8 to 3.5 mm above the board.
pcb_x = 3;
pcb_y = 3.5;
pcb_z = 9.2;
pcb_width = 78;
pcb_depth = 49;
pcb_thickness = 1.2;
pcb_corner_radius = 2;
pcb_top = pcb_z + pcb_thickness;

// Component datums taken from the vendor drawings. These set the stack, so a
// part substitution changes the enclosure and must change both files.
// KSC321G: 6.2 mm square body, 2.9 mm max to the body top, 3.5 mm to the
// actuator top, travel 0.2 +0.3/-0 mm at 2 +/-0.4 N (C&K KSC3 series).
switch_body = 6.2;
switch_body_height = 2.9;
switch_height = 3.5;
switch_actuator_diameter = 2.8;
switch_travel = 0.2;
switch_travel_max = 0.5;
// ESP32-S3-WROOM-1 including its shield; underside.
module_width = 25.5;
module_length = 18.0;
module_height = 3.1;
module_x = 3.5;
module_y = 14;
// Antenna end of the module, at the rear board edge. No copper, no metal.
antenna_x0 = 3;
antenna_x1 = 12;
antenna_y0 = 13;
antenna_y1 = 33;
// HRO TYPE-C-31-M-12, underside. Confirm against the vendor drawing.
usb_body_width = 8.94;
usb_body_height = 3.3;
// APBA2006 side-view LED, underside, emitting toward the front wall.
led_body_height = 1.0;
// Protected 503035-class pack, underside. 36 x 30 clears the published
// LP503035 drawing that the earlier 35 mm keep-out did not.
cell_x = 32;
cell_y = 6;
cell_width = 36;
cell_depth = 30;
cell_height = 5.4;

// Display datum and vendor envelope.
display_x = 42;
display_y = 37;
panel_width = 59.2;
panel_depth = 29.2;
panel_thickness = 1.0;
active_width = 48.55;
active_depth = 23.7046;
window_width = 50.6;
window_depth = 25.7;
lens_width = 55.0;
lens_depth = 30.1;
lens_thickness = 0.8;
lens_pocket_width = 55.4;
lens_pocket_depth = 30.5;

// Controls. Default Rev A presentation: Category sub-flush, Next flush.
// The cap sits in a bore through a thinned roof and is held up against a
// counterbore shoulder by the switch's own return spring. A brim under the
// roof takes an overload press into the shell rather than into the actuator.
category_x = 27;
category_y = 12;
category_bore = 10.30;
category_cap_diameter = 10.0;
category_proud = -0.20;
next_x = 53;
next_y = 12;
next_bore_width = 18.30;
next_bore_depth = 11.30;
next_bore_radius = 5.65;
next_cap_width = 18.0;
next_cap_depth = 11.0;
next_cap_radius = 5.5;
next_proud = 0.0;

// Cap construction, shared by both bores.
cap_bore_clearance = 0.15;     // radial gap between cap body and bore
cap_roof_thickness = 1.0;      // roof left above the counterbore shoulder
cap_flange_reach = 1.4;        // flange radius beyond the cap body
cap_counterbore_reach = 1.65;  // counterbore radius beyond the cap body
cap_brim_reach = 2.5;          // overload brim radius beyond the cap body
cap_brim_thickness = 0.4;
cap_overload_gap = 0.7;        // cap travel before the brim meets the roof
switch_pocket_clearance = 0.5; // around the switch body, inside the cap

// Front I/O. Both openings are derived from the underside mounting height, so
// they cannot drift from the parts they serve.
usb_x = 42;
usb_width = 9.4;
usb_height = 3.8;
usb_center_z = pcb_z - usb_body_height / 2;
light_pipe_x = 52;
light_pipe_diameter = 2.4;
light_pipe_collar_diameter = 5.0;
light_pipe_collar_length = 1.5;
light_pipe_center_z = pcb_z - led_body_height / 2;

// Display FPC. The flex leaves the panel past the board's front edge and
// folds under it into a bottom-side connector facing forward, which is why
// the retainer's exit slot sits at the front rather than the rear.
fpc_x = 26;
fpc_y = 47;
fpc_width = 15.3;
fpc_depth = 5.4;
fpc_exit_width = 12;

mount_points = [
    [13, 7], [77.5, 7], [13, 49], [77.5, 49]
];
service_points = [[6.5, 40], [77.5, 40]];
foot_points = [[14, 8], [70, 8], [14, 48], [70, 48]];

function face_z(y) = face_rear_z -
    (face_rear_z - face_front_z) *
    (y - corner_radius) /
    (case_depth - 2 * corner_radius);

// Inner surface of the roof directly above y.
function roof_z(y) = face_z(y) - roof;
// Shoulder the cap flange rests against, under a locally thinned roof.
function cap_shoulder_z(y) = face_z(y) - cap_roof_thickness;
// Top of the switch actuator, which the cap sits on.
function actuator_top_z() = pcb_top + switch_height;
// Underside of the overload brim's seat.
function cap_brim_top_z(y) = roof_z(y) - cap_overload_gap;

// A cap outline centred on the origin. A radius of half the smaller side
// gives a circle, so both bores use one profile.
module cap_2d(width, depth, radius) {
    hull()
        for (x = [-(width / 2 - radius), width / 2 - radius],
             y = [-(depth / 2 - radius), depth / 2 - radius])
            translate([x, y]) circle(r = radius);
}

module rounded_2d(width, depth, radius) {
    hull()
        for (x = [radius, width - radius], y = [radius, depth - radius])
            translate([x, y]) circle(r = radius);
}

module rounded_prism(width, depth, height, radius) {
    linear_extrude(height = height)
        rounded_2d(width, depth, radius);
}

module centered_rounded_prism(width, depth, height, radius) {
    translate([-width / 2, -depth / 2, -height / 2])
        rounded_prism(width, depth, height, radius);
}

module xy_centered_rounded_prism(width, depth, height, radius) {
    translate([-width / 2, -depth / 2, 0])
        rounded_prism(width, depth, height, radius);
}

module sloped_rounded_solid(inset = 0, bottom = seam_z, top_offset = 0) {
    local_radius = max(0.5, corner_radius - inset);
    x0 = corner_radius;
    x1 = case_width - corner_radius;
    y0 = corner_radius;
    y1 = case_depth - corner_radius;
    hull()
        for (x = [x0, x1], y = [y0, y1])
            translate([x, y, bottom])
                cylinder(
                    r = local_radius,
                    h = (y == y0 ? face_rear_z : face_front_z) +
                        top_offset - bottom
                );
}

module face_part(width, depth, thickness, radius, x, y, z_offset = 0) {
    translate([x, y, face_z(y) + z_offset])
        rotate([-face_angle, 0, 0])
            centered_rounded_prism(width, depth, thickness, radius);
}


// The cap bore is a straight hole through a locally thinned roof; the wider
// counterbore below it leaves the shoulder the cap flange is held against.
module cap_bore(x, y, width, depth, radius) {
    translate([x, y, cap_shoulder_z(y)])
        linear_extrude(height = 20)
            offset(r = cap_bore_clearance) cap_2d(width, depth, radius);
    translate([x, y, roof_z(y) - 0.01])
        linear_extrude(height = roof - cap_roof_thickness + 0.02)
            offset(r = cap_flange_reach + fit_clearance_xy)
                cap_2d(width, depth, radius);
}

module top_shell_blank() {
    difference() {
        sloped_rounded_solid();
        // Open-bottom cavity leaves conservative roof and wall thickness.
        sloped_rounded_solid(
            inset = wall,
            bottom = seam_z - 0.2,
            top_offset = -roof
        );

        // Lens rebate and the smaller through-window are separate operations.
        face_part(
            lens_pocket_width,
            lens_pocket_depth,
            1.25,
            1.2,
            display_x,
            display_y,
            -0.45
        );
        face_part(
            window_width,
            window_depth,
            7,
            1.0,
            display_x,
            display_y,
            -3.0
        );

        cap_bore(category_x, category_y,
                 category_cap_diameter, category_cap_diameter,
                 category_cap_diameter / 2);
        cap_bore(next_x, next_y, next_cap_width, next_cap_depth, next_cap_radius);

        // USB-C mouth and adjacent status-light channel through the front wall.
        translate([usb_x, case_depth + 1, usb_center_z])
            rotate([90, 0, 0])
                xy_centered_rounded_prism(
                    usb_width,
                    usb_height,
                    wall + 2,
                    usb_height / 2
                );
        translate([light_pipe_x, case_depth + 0.5, light_pipe_center_z])
            rotate([90, 0, 0])
                cylinder(d = light_pipe_diameter, h = wall + 3);
    }
}



module top_shell() {
    difference() {
        union() {
            top_shell_blank();
            // Standoffs the board sits on, and pillars from the roof that hold
            // it there. The board is captured by geometry: the screws below
            // only clamp the base and the steel to the shell.
            for (point = mount_points) {
                translate([point[0], point[1], seam_z])
                    cylinder(d = boss_outer_diameter, h = pcb_z - seam_z);
                translate([point[0], point[1], pcb_top])
                    cylinder(d = pillar_diameter, h = face_z(point[1]) - pcb_top);
            }
            // Collar that carries the light pipe across the cavity.
            translate([light_pipe_x, case_depth - wall, light_pipe_center_z])
                rotate([90, 0, 0])
                    cylinder(d = light_pipe_collar_diameter,
                             h = light_pipe_collar_length);
        }
        for (point = mount_points)
            translate([point[0], point[1], seam_z - 0.5])
                cylinder(d = boss_pilot_diameter, h = pcb_z - seam_z);
        // The pillar must not close the board's own mounting hole.
        for (point = mount_points)
            translate([point[0], point[1], pcb_top - 0.1])
                cylinder(d = mount_hole_diameter, h = 3);
        // Re-open the light-pipe channel through the collar.
        translate([light_pipe_x, case_depth + 0.5, light_pipe_center_z])
            rotate([90, 0, 0])
                cylinder(d = light_pipe_diameter,
                         h = wall + light_pipe_collar_length + 1);
    }
}


module base() {
    difference() {
        translate([wall - 0.8, wall - 0.8, base_z])
            rounded_prism(
                case_width - 2 * (wall - 0.8),
                case_depth - 2 * (wall - 0.8),
                base_thickness,
                2.0
            );

        for (point = concat(mount_points, service_points))
            translate([point[0], point[1], base_z - 0.2])
                cylinder(
                    d = point[1] == service_points[0][1] ? 2.5 : mount_hole_diameter,
                    h = base_thickness + 0.4
                );

        // Shallow datum only: the adhesive-backed cell is retained above it.
        translate([cell_x, cell_y, base_z + base_thickness - cell_recess_depth])
            rounded_prism(cell_width, cell_depth, 0.5, 1.5);
    }
}


module steel_skin() {
    difference() {
        translate([0.8, 0.8, steel_z])
            rounded_prism(
                case_width - 1.6,
                case_depth - 1.6,
                steel_thickness,
                2.3
            );
        for (point = concat(mount_points, service_points))
            translate([point[0], point[1], steel_z - 0.2])
                cylinder(d = 3.0, h = steel_thickness + 0.4);
        // RF keepout: no steel under or beside the module antenna.
        translate([antenna_x0 - 2, -0.1, steel_z - 0.2])
            cube([antenna_x1 - antenna_x0 + 4,
                  antenna_y1 + 2,
                  steel_thickness + 0.4]);
    }
}

module foot(x, y) {
    translate([x, y, 0]) cylinder(d = 8, h = foot_height);
}

module lens() {
    color([31 / 255, 34 / 255, 35 / 255, 0.82])
        face_part(
            lens_width,
            lens_depth,
            lens_thickness,
            1.0,
            display_x,
            display_y,
            -lens_thickness / 2 + 0.05
        );
}

module panel_reference() {
    color(paper_raised_color)
        face_part(
            panel_width,
            panel_depth,
            panel_thickness,
            0.7,
            display_x,
            display_y,
            -2.3
        );
}


module retainer() {
    // Front-loaded frame. The exit slot clears the panel flex where it folds
    // past the board's front edge to the underside connector.
    difference() {
        face_part(61.2, 31.2, 1.2, 1.2, display_x, display_y, -2.8);
        face_part(56.8, 26.8, 2.0, 0.8, display_x, display_y, -2.8);
        translate([fpc_x - fpc_exit_width / 2, case_depth - 12, 7])
            cube([fpc_exit_width, 12, 8]);
    }
}


// One cap body, built from the switch outwards: the underside meets the
// actuator, the flange is caught under the counterbore shoulder, and the brim
// hits the roof after cap_overload_gap so a hard press loads the shell.
module cap_body(x, y, width, depth, radius, proud) {
    top_z = face_z(y) + proud;
    shoulder = cap_shoulder_z(y);
    brim_top = cap_brim_top_z(y);
    pocket = switch_body + 2 * switch_pocket_clearance;
    difference() {
        union() {
            translate([0, 0, shoulder])
                linear_extrude(height = top_z - shoulder)
                    cap_2d(width, depth, radius);
            translate([0, 0, brim_top])
                linear_extrude(height = shoulder - brim_top)
                    offset(r = cap_flange_reach) cap_2d(width, depth, radius);
            translate([0, 0, brim_top - cap_brim_thickness])
                linear_extrude(height = cap_brim_thickness)
                    offset(r = cap_brim_reach) cap_2d(width, depth, radius);
        }
        // Clearance for the switch body; the ceiling of this pocket is the
        // face the actuator pushes on.
        translate([0, 0, brim_top - cap_brim_thickness - 1])
            linear_extrude(height = actuator_top_z() - brim_top
                                    + cap_brim_thickness + 1)
                square([pocket, pocket], center = true);
    }
}

module cap_at(x, y, width, depth, radius, proud, assembled) {
    if (assembled) translate([x, y, 0]) cap_body(x, y, width, depth, radius, proud);
    else translate([0, 0, -(cap_brim_top_z(y) - cap_brim_thickness)])
             cap_body(x, y, width, depth, radius, proud);
}

module category_cap(assembled = false, proud = category_proud) {
    cap_at(category_x, category_y,
           category_cap_diameter, category_cap_diameter,
           category_cap_diameter / 2, proud, assembled);
}


module next_cap(assembled = false, proud = next_proud) {
    cap_at(next_x, next_y, next_cap_width, next_cap_depth,
           next_cap_radius, proud, assembled);
}

// Reference volume for a fitted KSC321G, so the assembly shows the real
// relationship between actuator, cap and roof.
module switch_reference(x, y) {
    color([0.15, 0.15, 0.16])
        translate([x, y, pcb_top]) {
            translate([-switch_body / 2, -switch_body / 2, 0])
                cube([switch_body, switch_body, switch_body_height]);
            translate([0, 0, switch_body_height])
                cylinder(d = switch_actuator_diameter,
                         h = switch_height - switch_body_height);
        }
}


module light_pipe() {
    // Spans the whole gap from the outer face to the LED at the board's front
    // edge; a 2 mm rod cannot bridge that unsupported, hence the shell collar.
    length = case_depth - (pcb_y + pcb_depth);
    color([0.65, 0.95, 0.82, 0.85])
        translate([light_pipe_x, case_depth, light_pipe_center_z])
            rotate([90, 0, 0])
                cylinder(d = light_pipe_diameter - 2 * 0.2, h = length);
}


module pcb_reference() {
    color([0.07, 0.25, 0.20])
        translate([pcb_x, pcb_y, pcb_z])
            rounded_prism(
                pcb_width,
                pcb_depth,
                pcb_thickness,
                pcb_corner_radius
            );

    // Underside parts, drawn where they actually sit.
    color([0.20, 0.20, 0.22])
        translate([module_x, module_y, pcb_z - module_height])
            cube([module_width, module_length, module_height]);
    color([0.62, 0.62, 0.65])
        translate([usb_x, pcb_y + pcb_depth - 4.5, usb_center_z])
            rotate([90, 0, 0])
                xy_centered_rounded_prism(usb_body_width, usb_body_height,
                                          7.3, usb_body_height / 2);
    color([0.85, 0.85, 0.3])
        translate([light_pipe_x - 1.0, pcb_y + pcb_depth - 2.0,
                   pcb_z - led_body_height])
            cube([2.0, 2.0, led_body_height]);
    color([0.20, 0.20, 0.22])
        translate([fpc_x - fpc_width / 2, fpc_y - fpc_depth / 2,
                   pcb_z - 1.25])
            cube([fpc_width, fpc_depth, 1.25]);

    switch_reference(category_x, category_y);
    switch_reference(next_x, next_y);

    // Battery and antenna keepouts are translucent design volumes.
    color([0.25, 0.25, 0.28, 0.60])
        translate([cell_x, cell_y, base_z + base_thickness - cell_recess_depth])
            rounded_prism(cell_width, cell_depth, cell_height, 2);
    color([0.85, 0.25, 0.20, 0.24])
        translate([antenna_x0, antenna_y0, pcb_z - module_height - 1])
            cube([antenna_x1 - antenna_x0,
                  antenna_y1 - antenna_y0,
                  module_height + 1 + pcb_thickness + 2]);
}


module gasket_reference() {
    // One splash loop per bore, clearing the cap brim. The break at the low
    // point of each loop drains rather than seals; no ingress rating is
    // claimed for the enclosure.
    for (cap = [[category_x, category_y, category_cap_diameter,
                 category_cap_diameter, category_cap_diameter / 2],
                [next_x, next_y, next_cap_width, next_cap_depth,
                 next_cap_radius]])
        color([0.12, 0.12, 0.12, 0.75])
            translate([cap[0], cap[1], roof_z(cap[1]) - 0.5])
                linear_extrude(height = 0.5)
                    difference() {
                        offset(r = cap_brim_reach + 2.2)
                            cap_2d(cap[2], cap[3], cap[4]);
                        offset(r = cap_brim_reach + 0.6)
                            cap_2d(cap[2], cap[3], cap[4]);
                        translate([-1, 0]) square([2, cap[3] / 2 + 6]);
                    }
}

module assembly(exploded = 0) {
    color(paper_color)
        translate([0, 0, 4 * exploded]) top_shell();
    color(paper_color)
        translate([0, 0, -2 * exploded]) base();
    color([0.34, 0.35, 0.36])
        translate([0, 0, -4 * exploded]) steel_skin();
    color([0.08, 0.08, 0.08])
        for (point = foot_points)
            translate([0, 0, -4 * exploded]) foot(point[0], point[1]);
    translate([0, 0, 3 * exploded]) lens();
    translate([0, 0, 1.5 * exploded]) panel_reference();
    color(ink_soft_color)
        translate([0, 0, 0.5 * exploded]) retainer();
    color(ink_color)
        translate([0, 0, 4 * exploded]) category_cap(true);
    color(ink_color)
        translate([0, 0, 4 * exploded]) next_cap(true);
    translate([0, 0, 4 * exploded]) light_pipe();
    // Always drawn: the section view is only useful with the board in it.
    translate([0, 0, -0.5 * exploded]) pcb_reference();
    if (exploded == 0)
        gasket_reference();
}

module section_view() {
    extent = 200;
    section_position = section_position_percent / 100 * (
        section_axis == "x" ? case_width :
        section_axis == "y" ? case_depth :
        face_rear_z
    );
    intersection() {
        assembly();
        if (section_axis == "x")
            translate([
                section_keep == "positive" ? section_position : -extent,
                -extent,
                -extent
            ])
                cube([
                    section_keep == "positive" ? extent : section_position + extent,
                    2 * extent,
                    2 * extent
                ]);
        else if (section_axis == "y")
            translate([
                -extent,
                section_keep == "positive" ? section_position : -extent,
                -extent
            ])
                cube([
                    2 * extent,
                    section_keep == "positive" ? extent : section_position + extent,
                    2 * extent
                ]);
        else if (section_axis == "z")
            translate([
                -extent,
                -extent,
                section_keep == "positive" ? section_position : -extent
            ])
                cube([
                    2 * extent,
                    2 * extent,
                    section_keep == "positive" ? extent : section_position + extent
                ]);
        else
            assert(false, str("Unknown section axis: ", section_axis));
    }
}


// A section of roof at the button row, with the real bore, counterbore and
// retention shoulder. Print it before the shell and check that a cap drops in,
// is held by the shoulder, and returns.
module coupon_buttons() {
    caps = [[13, category_cap_diameter, category_cap_diameter,
             category_cap_diameter / 2],
            [36, next_cap_width, next_cap_depth, next_cap_radius]];
    difference() {
        rounded_prism(50, 24, roof, 2);
        for (c = caps) {
            translate([c[0], 12, roof - cap_roof_thickness])
                linear_extrude(height = cap_roof_thickness + 0.1)
                    offset(r = cap_bore_clearance) cap_2d(c[1], c[2], c[3]);
            translate([c[0], 12, -0.05])
                linear_extrude(height = roof - cap_roof_thickness + 0.05)
                    offset(r = cap_flange_reach + fit_clearance_xy)
                        cap_2d(c[1], c[2], c[3]);
        }
    }
}



// Height from the brim's underside to the flange top, which is what the
// counterbore shoulder catches. Independent of y.
function cap_export_flange_top() =
    roof - cap_roof_thickness + cap_overload_gap + cap_brim_thickness;

module coupon_buttons_assembly() {
    lift = (roof - cap_roof_thickness) - cap_export_flange_top();
    color(paper_color) coupon_buttons();
    color(ink_color) translate([13, 12, lift]) category_cap(false);
    color(ink_color) translate([36, 12, lift]) next_cap(false);
}


module coupon_usb() {
    // Front-wall section at the real opening heights, cut from the same
    // datums the shell uses.
    z0 = light_pipe_center_z - 4;
    difference() {
        translate([0, 0, z0]) rounded_prism(34, 12, 8, 2);
        translate([17, 12.1, usb_center_z])
            rotate([90, 0, 0])
                xy_centered_rounded_prism(
                    usb_width,
                    usb_height,
                    12.2,
                    usb_height / 2
                );
        translate([27, 12.1, light_pipe_center_z])
            rotate([90, 0, 0])
                cylinder(d = light_pipe_diameter, h = 12.2);
    }
}

module coupon_lens() {
    difference() {
        rounded_prism(64, 38, 2.5, 2);
        translate([4.3, 3.75, 1.65])
            rounded_prism(lens_pocket_width, lens_pocket_depth, 1.0, 1.2);
        translate([6.7, 6.15, -0.1])
            rounded_prism(window_width, window_depth, 2.8, 1.0);
    }
}


// Gauges the real standoff, not a stand-in: same height, diameter and pilot as
// the shell builds.
module coupon_boss() {
    difference() {
        union() {
            rounded_prism(22, 18, 2.4, 2);
            translate([11, 9, 2.4])
                cylinder(d = boss_outer_diameter, h = pcb_z - seam_z);
        }
        translate([11, 9, 2.0])
            cylinder(d = boss_pilot_diameter, h = pcb_z - seam_z);
    }
}


// Geometry contract. These fail the render, so `just hw-case-check` catches a
// parameter change that breaks the stack instead of exporting a bad STL.
cell_top_z = base_z + base_thickness - cell_recess_depth + cell_height;
assert(cell_top_z <= pcb_z,
       str("Cell keep-out reaches ", cell_top_z, " mm; the board is at ", pcb_z));
assert(pcb_z - module_height > seam_z,
       "The module does not clear the base inside the cell cavity");
assert(module_x + module_width <= cell_x,
       "The module overlaps the cell keep-out in plan");
assert(usb_center_z - usb_height / 2 > seam_z,
       "The USB opening reaches below the case seam");
assert(usb_center_z + usb_height / 2 < face_front_z - roof,
       "The USB opening reaches into the front roof");
assert(light_pipe_center_z + light_pipe_diameter / 2 < pcb_top,
       "The light pipe is not aligned with an underside LED");
assert(light_pipe_center_z - light_pipe_collar_diameter / 2 > seam_z,
       "The light-pipe collar reaches below the case seam");

for (point = mount_points) {
    assert(point[0] > antenna_x1 - 2.5 || point[0] > 60,
           str("Mounting point ", point, " sits in the module antenna keep-out"));
    assert(roof_z(point[1]) > pcb_top,
           str("No room for a hold-down pillar at y=", point[1]));
    assert(point[0] < module_x || point[0] > module_x + module_width
           || point[1] < module_y || point[1] > module_y + module_length,
           str("Mounting point ", point, " lands on the module"));
    assert(point[0] < cell_x || point[0] > cell_x + cell_width
           || point[1] < cell_y || point[1] > cell_y + cell_depth,
           str("Mounting point ", point, " lands on the cell"));
}

for (button = [[category_y, category_cap_diameter, category_proud],
               [next_y, next_cap_width, next_proud]]) {
    assert(cap_shoulder_z(button[0]) > actuator_top_z(),
           "The cap has no material between the actuator and the shoulder");
    assert(cap_brim_top_z(button[0]) - cap_brim_thickness > pcb_top,
           "The overload brim would touch the board");
    assert(actuator_top_z() < face_z(button[0]) + button[2],
           "The switch actuator would stand proud of the cap");
    assert(switch_body + 2 * switch_pocket_clearance
           < button[1] + 2 * cap_flange_reach,
           "The switch pocket is wider than the cap flange");
    assert(cap_overload_gap > switch_travel_max,
           "The overload brim stops the cap before the switch can operate");
}

assert(window_width < lens_width && lens_width < lens_pocket_width,
       "Window, lens and lens rebate are not nested");
assert(active_width < window_width && active_depth < window_depth,
       "The visible window does not clear the panel's active area");

echo(str("CICALA_REV=A; part=", part));
echo(str("ENVELOPE_MM=", case_width, "x", case_depth, "x", face_rear_z));
echo(str("PCB_MM=", pcb_width, "x", pcb_depth, "x", pcb_thickness));
echo(str("PCB_ORIGIN_MM=", pcb_x, ",", pcb_y, ",", pcb_z));
echo(str("CELL_CLEARANCE_MM=", pcb_z - cell_top_z));
echo(str("CAP_TRAVEL_TO_STOP_MM=", cap_overload_gap));

if (part == "assembly") assembly(0);
else if (part == "exploded") assembly(4);
else if (part == "section") section_view();
else if (part == "top_shell") top_shell();
else if (part == "base") base();
else if (part == "retainer") retainer();
else if (part == "category_cap") category_cap(false);
else if (part == "next_cap") next_cap(false);
else if (part == "lens") translate([-display_x, -display_y, -face_z(display_y)]) lens();
else if (part == "steel_skin") steel_skin();
else if (part == "light_pipe") translate([-light_pipe_x, -case_depth, -light_pipe_center_z]) light_pipe();
else if (part == "pcb_reference") pcb_reference();
else if (part == "coupon_buttons") coupon_buttons();
else if (part == "coupon_buttons_assembly") coupon_buttons_assembly();
else if (part == "coupon_usb") coupon_usb();
else if (part == "coupon_lens") coupon_lens();
else if (part == "coupon_boss") coupon_boss();
else assert(false, str("Unknown part: ", part));
