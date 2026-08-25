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
boss_pilot_diameter = 2.1;

// PCB datum: rear-left corner at (3, 3), component side upward.
pcb_x = 3;
pcb_y = 3;
pcb_z = 9.2;
pcb_width = 78;
pcb_depth = 45;
pcb_thickness = 1.2;
pcb_corner_radius = 2;

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

// Front I/O. Light pipe is sealed by a gasket or adhesive at assembly.
usb_x = 42;
usb_width = 9.4;
usb_height = 3.8;
usb_center_z = 7.7;
light_pipe_x = 52;
light_pipe_diameter = 2.4;
light_pipe_center_z = 7.7;

mount_points = [
    [6.5, 6.5], [77.5, 6.5], [6.5, 44.5], [77.5, 44.5]
];
service_points = [[6.5, 28], [77.5, 28]];
foot_points = [[14, 8], [70, 8], [14, 48], [70, 48]];

function face_z(y) = face_rear_z -
    (face_rear_z - face_front_z) *
    (y - corner_radius) /
    (case_depth - 2 * corner_radius);

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

        // Vertical cap bores. Flange and actuator stack are coupon-gated.
        translate([category_x, category_y, seam_z - 1])
            cylinder(d = category_bore, h = 20);
        translate([next_x, next_y, seam_z + 8])
            centered_rounded_prism(
                next_bore_width,
                next_bore_depth,
                20,
                next_bore_radius
            );

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
            // Bosses are rebuilt inside the cavity after the cavity subtraction.
            for (point = mount_points)
                translate([point[0], point[1], seam_z])
                    cylinder(d = boss_outer_diameter, h = 4.4);
        }
        for (point = mount_points)
            translate([point[0], point[1], seam_z - 0.5])
                cylinder(d = boss_pilot_diameter, h = 6);
    }
}

module base() {
    difference() {
        translate([1.2, 1.2, base_z])
            rounded_prism(
                case_width - 2.4,
                case_depth - 2.4,
                base_thickness,
                2.0
            );

        for (point = concat(mount_points, service_points))
            translate([point[0], point[1], base_z - 0.2])
                cylinder(d = point[1] == 28 ? 2.5 : 2.7, h = base_thickness + 0.4);

        // Shallow datum only: the adhesive-backed cell is retained above it.
        translate([24.5, 4.0, base_z + base_thickness - 0.35])
            rounded_prism(35, 30, 0.5, 1.5);
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
        // RF keepout: no steel under or beside the PCB antenna zone.
        translate([-0.1, 20, steel_z - 0.2])
            cube([15.1, 16, steel_thickness + 0.4]);
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
    // Front-loaded frame with a clear FPC exit at the rear-left of the panel.
    difference() {
        face_part(61.2, 31.2, 1.2, 1.2, display_x, display_y, -2.8);
        face_part(56.8, 26.8, 2.0, 0.8, display_x, display_y, -2.8);
        translate([24, 47, 7]) cube([12, 6, 8]);
    }
}

module category_cap(assembled = false, proud = category_proud) {
    local_z = assembled ? face_z(category_y) + proud - 1.15 : 0;
    local_x = assembled ? category_x : 0;
    local_y = assembled ? category_y : 0;
    translate([local_x, local_y, local_z]) {
        cylinder(d = category_cap_diameter, h = 1.15);
        translate([0, 0, -1.15]) cylinder(d = 13.0, h = 1.15);
        translate([0, 0, -2.8]) cylinder(d = 4.2, h = 1.7);
    }
}

module next_cap(assembled = false, proud = next_proud) {
    local_z = assembled ? face_z(next_y) + proud - 1.15 : 0;
    local_x = assembled ? next_x : 0;
    local_y = assembled ? next_y : 0;
    translate([local_x, local_y, local_z]) {
        xy_centered_rounded_prism(
            next_cap_width,
            next_cap_depth,
            1.15,
            next_cap_radius
        );
        translate([0, 0, -1.15])
            xy_centered_rounded_prism(21, 14, 1.15, 6.8);
        translate([0, 0, -2.8]) cylinder(d = 4.2, h = 1.7);
    }
}

module light_pipe() {
    color([0.65, 0.95, 0.82, 0.85])
        translate([light_pipe_x, case_depth, light_pipe_center_z])
            rotate([90, 0, 0])
                cylinder(d = 2.0, h = wall + 1.2);
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

    // Battery and antenna keepouts are translucent design volumes.
    color([0.25, 0.25, 0.28, 0.60])
        translate([24.5, 4, base_z + base_thickness - 0.35])
            rounded_prism(35, 30, 5.4, 2);
    color([0.85, 0.25, 0.20, 0.24])
        translate([0, 20, pcb_z]) cube([15, 16, 6.3]);
}

module gasket_reference() {
    color([0.12, 0.12, 0.12, 0.75])
        translate([18, 4, face_z(12) - 2.5])
            difference() {
                rounded_prism(44, 16, 0.5, 4);
                translate([2, 2, -0.1]) rounded_prism(40, 12, 0.7, 3);
                // Low-point drain breaks the outer loop intentionally.
                translate([22, 11, -0.1]) cube([2, 6, 0.7]);
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
    if (exploded > 0)
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

module coupon_buttons() {
    difference() {
        rounded_prism(50, 24, 3.0, 2);
        translate([13, 12, -0.1]) cylinder(d = category_bore, h = 3.2);
        translate([36, 12, 1.5])
            centered_rounded_prism(
                next_bore_width,
                next_bore_depth,
                3.2,
                next_bore_radius
            );
    }
}

module coupon_buttons_assembly() {
    color(paper_color) coupon_buttons();
    color(ink_color) translate([13, 12, 4.35]) category_cap(false);
    color(ink_color) translate([36, 12, 4.35]) next_cap(false);
}

module coupon_usb() {
    difference() {
        rounded_prism(34, 12, 8, 2);
        translate([17, 12.1, 4])
            rotate([90, 0, 0])
                xy_centered_rounded_prism(
                    usb_width,
                    usb_height,
                    12.2,
                    usb_height / 2
                );
        translate([27, 12.1, 4])
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

module coupon_boss() {
    difference() {
        union() {
            rounded_prism(22, 18, 2.4, 2);
            translate([11, 9, 2.4]) cylinder(d = boss_outer_diameter, h = 6);
        }
        translate([11, 9, 2.0]) cylinder(d = boss_pilot_diameter, h = 7);
    }
}

echo(str("CICALA_REV=A; part=", part));
echo(str("ENVELOPE_MM=", case_width, "x", case_depth, "x", face_rear_z));
echo(str("PCB_MM=", pcb_width, "x", pcb_depth, "x", pcb_thickness));
echo(str("PCB_ORIGIN_MM=", pcb_x, ",", pcb_y, ",", pcb_z));

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
