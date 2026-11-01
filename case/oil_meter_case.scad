// Oil meter case: FireBeetle 2 ESP32-C5 + JSN-SR04T driver board + 1-cell LiPo
//
// Layout (top view, lid removed):
//
//              BOOT rod
//   +-----------||--------------------------------+
//   |                              <-Dupont-+---+ |
//   USB-C [ FireBeetle 2 C5 ]               |JSN| |  probe plug from the top,
//   |                                       +---+ |  through the lid
//   +-----------||--------------------------------+
//              RST rod
//
//   The battery (36 x 70 x 10) sits in a frame on the underside of the lid, above the
//   FireBeetle. Its JST-PH lead goes down to the BAT connector of the FireBeetle.
//
//   The JSN-SR04T board is mounted upside down on tall standoffs, components (10 mm)
//   towards the floor, so the probe socket on its back points up into an opening in the
//   lid: the probe cable plugs in from outside without a bend. Its right-angle header
//   points towards the FireBeetle.
//
// Parts (set `part`, or use the OpenSCAD customizer):
//   base    - the box, print open side up
//   lid     - print outer face down (battery frame up)
//   buttons - two push-rods for RST and BOOT, print standing on the collar
//   assembly- preview with dummy boards and battery (not for printing)
//
// Hardware: 4x M3x10 self-tapping (or M3 + heat-set inserts, see boss_hole_d),
//           2x M2x5 self-tapping for the FireBeetle, 4x M2.5x8 self-tapping for the JSN board.
//
// FireBeetle dimensions are taken from DFRobot's DFR1222 CAD drawing. JSN-SR04T V3.3
// dimensions from the board (41 x 28.5) and the seller photos.

/* [Part] */
part = "assembly"; // [assembly, base, lid, buttons, all]

/* [General] */
wall      = 2.0;
floor_t   = 2.0;
lid_t     = 2.5;
clr       = 0.3;   // fit clearance
corner_r  = 2.0;   // outer corner radius

/* [Battery] */
bat = [70, 36, 10];  // length, width, thickness

/* [FireBeetle 2 ESP32-C5 (DFR1222)] */
fb_l           = 60;
fb_w           = 25.4;
fb_pcb         = 1.6;
fb_h           = 7.3;    // pcb bottom to top of tallest part (BAT connector)
fb_hole_d      = 2.0;    // M2
fb_hole_inset  = 1.7;    // hole centres from each edge (22 x 56.6 pattern)
fb_standoff    = 4;      // room for solder joints / wire ends underneath
fb_btn_from_end= 6.77;   // RST/BOOT plunger centre, measured from the USB end
fb_btn_z       = 1.8;    // plunger centre above pcb top
fb_usb_w       = 9.0;
fb_usb_h       = 3.3;
fb_usb_out     = 1.2;    // USB-C sticks out past the pcb edge
fb_bat_conn    = [10.5, 18.5]; // BAT connector span from USB end (on the BOOT side)

/* [JSN-SR04T V3.3 driver board] */
// Board coordinates: u along the 41 mm edge, v from the header edge (28.5 mm).
sr_l        = 41;
sr_w        = 28.5;
sr_pcb      = 1.6;
sr_holes    = [[2.5, 2.5], [38.5, 2.5], [2.5, 26.0], [38.5, 26.0]]; // [u, v]
sr_pilot_d  = 2.2;    // M2.5 self-tapping into the standoffs
sr_comp_clear = 11;   // parts on the component (header) side
sr_jst      = [20.5, 13.4];  // [u, v] centre of the probe socket on the back
sr_jst_h    = 8.0;    // JST socket height above the pcb back (unplugged)
sr_jst_hole = [8.5, 10];   // cutout for socket/plug [along v, along u]
sr_plug_reach = 10;   // the plug can reach this far into the case
dupont_clear= 20;     // header pins + Dupont plugs + wire bend, beyond the header edge
dupont_len  = 15;     // rigid part of the Dupont plugs beyond the header edge

/* [Openings] */
usb_hole      = [12.5, 7.5];   // USB-C plug overmould

/* [Buttons] */
rod_d     = 2.4;
rod_hole_d= 2.9;
collar_d  = 4.5;
collar_t  = 1.0;
rod_gap   = 0.6;  // collar to plunger at rest
rod_out   = 1.0;  // rod sticks out of the wall this much

/* [Mounting] */
boss_d     = 8;
boss_hole_d= 2.6;   // 2.6 for M3 self-tapping, 4.0 for M3 heat-set inserts
lid_hole_d = 3.4;
lid_head_d = 6.2;   // counterbore for the screw head
tab_mount  = true;  // [true, false]
tab_w      = 12;
tab_len    = 10;
tab_t      = 3;
tab_hole_d = 4.5;

$fn = 48;

// ---------------------------------------------------------------- layout
lip_t   = 1.2;                      // lid locating lip
frame_t = 1.2;                      // battery frame wall
fb_x0 = fb_usb_out + 0.3;           // pcb edge at the USB end
fb_y0 = 5;                          // RST side gap (push-rod)

// battery (hanging from the lid) next to the USB end
bat_x0 = clr + lip_t + frame_t + clr;
bat_x1 = bat_x0 + bat[0] + 2 * clr + frame_t;

// JSN board past the battery (its socket must not be covered), header edge facing the
// FireBeetle. The Dupont plugs are at the height of the FireBeetle's module: keep clear.
sr_floor_h = sr_comp_clear + 0.5;   // standoff height under the components
sr_back_z  = sr_floor_h + sr_pcb;   // pcb back (socket side), facing up
sr_x0 = max(bat_x1 + 1, fb_x0 + fb_l + dupont_len + 1) - 11; // minus allowed overlap

in_l = sr_x0 + sr_w + clr + lip_t + 0.5;   // clear of the lid lip
in_w = max(fb_y0 + fb_w + 13.6, bat[1] + 2 * (clr + lip_t + frame_t + clr), sr_l + 3);
in_h = max(fb_h + fb_standoff + bat[2] + 3, sr_back_z + sr_jst_h + 0.5, sr_back_z + bat[2] + 2);

bat_y0 = (in_w - bat[1]) / 2 - clr;
sr_y0 = (in_w - sr_l) / 2;

fb_top = fb_standoff + fb_pcb;      // pcb top above inner floor
btn_x  = fb_x0 + fb_btn_from_end;
btn_z  = fb_top + fb_btn_z;

out_l = in_l + 2 * wall;
out_w = in_w + 2 * wall;
out_h = in_h + floor_t;

// rod guides: from the wall to collar_t + rod_gap before the board edge
rst_guide  = fb_y0 - collar_t - rod_gap;
boot_guide = in_w - (fb_y0 + fb_w) - collar_t - rod_gap;
rst_rod_len  = rod_out + wall + rst_guide;
boot_rod_len = rod_out + wall + boot_guide;

boss_off = 0;  // boss centre outside the outer corner
boss_pos = [for (x = [-wall - boss_off, in_l + wall + boss_off],
                 y = [-wall - boss_off, in_w + wall + boss_off]) [x, y]];

echo(str("Outer size: ", out_l + 2 * (boss_off + boss_d / 2), " x ",
         out_w + 2 * tab_len, " (incl. tabs) x ", out_h + lid_t," mm (inner ", in_l, " x ", in_w, " x ", in_h, ")"));
assert(sr_x0 - dupont_clear > fb_x0 + 30, "Dupont plugs would reach the FireBeetle's tall parts");
assert(in_h + lid_t - (sr_back_z + sr_jst_h) <= sr_plug_reach,
       "probe plug can't reach the socket through the lid");
echo(str("Probe socket top ", in_h + lid_t - (sr_back_z + sr_jst_h), " mm below the lid surface"));
echo(str("Push-rods: RST ", rst_rod_len, " mm, BOOT ", boot_rod_len, " mm"));

// ---------------------------------------------------------------- helpers
module rrect(size, r) {
    translate([r, r]) offset(r) square([size[0] - 2 * r, size[1] - 2 * r]);
}

module outline2d() {
    translate([-wall, -wall]) rrect([out_l, out_w], corner_r);
    for (p = boss_pos) translate(p) circle(d = boss_d);
}

module standoff(h, hole_d, d = 0) {
    od = d > 0 ? d : hole_d + 3.2;
    difference() {
        cylinder(d = od, h = h);
        translate([0, 0, 1]) cylinder(d = hole_d * 0.85, h = h);
    }
}

// ---------------------------------------------------------------- base
module base() {
    difference() {
        union() {
            translate([0, 0, -floor_t]) linear_extrude(out_h) outline2d();
            // wall-mounting tabs on the long sides
            if (tab_mount)
                for (x = [in_l * 0.2, in_l * 0.8], s = [0, 1])
                    translate([x, s ? in_w + wall : -wall, -floor_t])
                        mirror([0, s ? 0 : 1, 0]) mount_tab();
        }
        // cavity
        cube([in_l, in_w, in_h + 1]);
        // lid screw holes
        for (p = boss_pos) translate([p[0], p[1], 2 - floor_t])
            cylinder(d = boss_hole_d, h = out_h);
        // USB-C
        translate([-wall - 1, fb_y0 + fb_w / 2, fb_top + fb_usb_h / 2])
            rotate([0, 90, 0]) linear_extrude(wall + 2)
                rrect_c([usb_hole[1], usb_hole[0]], 2);
        // push-rod holes (RST on the -Y wall, BOOT on the +Y wall)
        translate([btn_x, -wall - 1, btn_z]) rotate([-90, 0, 0])
            cylinder(d = rod_hole_d, h = wall + rst_guide + 2);
        translate([btn_x, in_w - boot_guide - 1, btn_z]) rotate([-90, 0, 0])
            cylinder(d = rod_hole_d, h = wall + boot_guide + 2);
        // labels
        label([btn_x, -wall, btn_z + 4.5], 0, "RST");
        label([btn_x, in_w + wall, btn_z + 4.5], 180, "BOOT");
    }
    // FireBeetle standoffs
    for (p = fb_holes()) translate([p[0], p[1], 0])
        standoff(fb_standoff, fb_hole_d, 4.5);
    // JSN-SR04T standoffs, up from the floor to the component side
    sr_frame() for (h = sr_holes) translate([h[1], h[0], sr_pcb])
        difference() {
            cylinder(d = 5, h = sr_floor_h + 0.5);
            translate([0, 0, -1]) cylinder(d = sr_pilot_d, h = min(9, sr_floor_h));
        }
    // push-rod guides
    rod_guide(0, rst_guide);
    rod_guide(in_w - boot_guide, boot_guide);
}

module rrect_c(size, r) { translate(-size / 2) rrect(size, r); }

module label(pos, rot, txt) {
    translate(pos) rotate([90, 0, rot]) translate([0, 0, -0.4])
        linear_extrude(1) text(txt, size = 3.2, halign = "center", valign = "center",
                               font = "Liberation Sans:style=Bold");
}

module mount_tab() {
    difference() {
        hull() {
            translate([-tab_w / 2, 0, 0]) cube([tab_w, 0.01, tab_t]);
            translate([0, tab_len - tab_w / 2, 0]) cylinder(d = tab_w, h = tab_t);
        }
        translate([0, tab_len - tab_w / 2, -1]) cylinder(d = tab_hole_d, h = tab_t + 2);
    }
}

module rod_guide(y, len) {
    if (len > 0.5)
        difference() {
            translate([btn_x - 2.5, y, 0]) cube([5, len, btn_z + 2.5]);
            translate([btn_x, y - 1, btn_z]) rotate([-90, 0, 0])
                cylinder(d = rod_hole_d, h = len + 2);
        }
}

// Board coordinates of the JSN board (v from the header edge, u along the 41 mm edge,
// pcb back = socket side at z = 0, component side at z = +sr_pcb) -> case:
// v -> +X, u -> -Y, component side down.
module sr_frame() {
    multmatrix([[1, 0, 0, sr_x0],
                [0, -1, 0, sr_y0 + sr_l],
                [0, 0, -1, sr_back_z],
                [0, 0, 0, 1]]) children();
}

// Lid placement: maps lid (print) coordinates to case coordinates and back.
module lid_to_case() {
    translate([0, in_w, in_h + lid_t]) mirror([0, 1, 0]) mirror([0, 0, 1]) children();
}

// Cut-outs in the lid, drawn in case coordinates.
module lid_cuts_case() {
    jst = [sr_x0 + sr_jst[1], sr_y0 + sr_l - sr_jst[0]];
    translate([jst[0], jst[1], in_h - 1]) linear_extrude(lid_t + 2)
        rrect_c([sr_jst_hole[0], sr_jst_hole[1]], 1);
    translate([jst[0] + sr_jst_hole[0] / 2 + 3.5, jst[1], in_h + lid_t - 0.6])
        linear_extrude(1) rotate(90) text("PROBE", size = 3.2, halign = "center",
            valign = "center", font = "Liberation Sans:style=Bold");
}

function fb_holes() = [for (x = [fb_hole_inset, fb_l - fb_hole_inset],
                            y = [fb_hole_inset, fb_w - fb_hole_inset])
                       [fb_x0 + x, fb_y0 + y]];

// ---------------------------------------------------------------- lid
// Modelled in print orientation: outer face on z = 0, battery frame pointing up.
module lid() {
    lip_h = 2;
    frame_h = 6;
    fi = [bat[0] + 2 * clr, bat[1] + 2 * clr];
    difference() {
        union() {
            linear_extrude(lid_t) outline2d();
            // locating lip inside the walls
            translate([clr, clr, lid_t]) linear_extrude(lip_h) difference() {
                square([in_l - 2 * clr, in_w - 2 * clr]);
                translate([lip_t, lip_t]) square([in_l - 2 * clr - 2 * lip_t,
                                                   in_w - 2 * clr - 2 * lip_t]);
            }
            // battery frame, open at the short ends for the lead and for removal
            translate([bat_x0 - frame_t, bat_y0 - frame_t, lid_t])
                difference() {
                    cube([fi[0] + 2 * frame_t, fi[1] + 2 * frame_t, frame_h]);
                    translate([frame_t, frame_t, -1]) cube([fi[0], fi[1], frame_h + 2]);
                    for (x = [-1, fi[0] + frame_t - 1])
                        translate([x, frame_t + fi[1] / 2 - 7, -1])
                            cube([frame_t + 2, 14, frame_h + 2]);
                }
        }
        for (p = boss_pos) translate([p[0], p[1], -1]) {
            cylinder(d = lid_hole_d, h = lid_t + 2);
            cylinder(d = lid_head_d, h = 1 + 1.2);
        }
        lid_to_case() lid_cuts_case();
    }
}

// ---------------------------------------------------------------- buttons
module push_rod(len) {
    cylinder(d = collar_d, h = collar_t);
    cylinder(d = rod_d, h = collar_t + len);
}

module buttons() {
    push_rod(rst_rod_len);
    translate([8, 0, 0]) push_rod(boot_rod_len);
}

// ---------------------------------------------------------------- preview
module dummy_firebeetle() {
    translate([fb_x0, fb_y0, fb_standoff]) {
        color("#222") difference() {
            cube([fb_l, fb_w, fb_pcb]);
            for (y = [-0.1, fb_w - 1.1]) translate([fb_btn_from_end - 2.25, y, -1])
                cube([4.5, 1.2, fb_pcb + 2]);
        }
        color("silver") translate([-fb_usb_out, fb_w / 2 - fb_usb_w / 2, fb_pcb])
            cube([7.5, fb_usb_w, fb_usb_h]);
        color("ivory") for (y = [0.1, fb_w - 1.2]) translate([fb_btn_from_end - 2.25, y, fb_pcb])
            cube([4.5, 1.1, 3]);
        color("white") translate([fb_bat_conn[0], fb_w - 5.1, fb_pcb])
            cube([fb_bat_conn[1] - fb_bat_conn[0], 5.1, fb_h - fb_pcb]);
        color("#bbb") translate([fb_l - 25.5, fb_w / 2 - 9, fb_pcb]) cube([25.5, 18, 3.2]);
    }
}

module dummy_jsn() {
    sr_frame() {
        color("royalblue") difference() {
            cube([sr_w, sr_l, sr_pcb]);
            for (h = sr_holes) translate([h[1], h[0], -1]) cylinder(d = 3, h = 4);
        }
        // positions from the seller photos
        color("#bbb") translate([15.4, 0.7, sr_pcb]) cube([8, 7.7, 10]);      // coil
        color("#333") translate([11.6, 4.9, sr_pcb]) cylinder(d = 4, h = 7);  // caps
        color("#333") translate([7.1, 3.5, sr_pcb]) cylinder(d = 4, h = 7);
        color("black") translate([0, 16.3, sr_pcb]) cube([2.5, 10, 2.5]);    // header
        color("gold") for (u = [17.5, 20, 22.5, 25])
            translate([-6, u, sr_pcb + 1.2]) cube([6, 0.6, 0.6]);
        color("#222", 0.6) translate([-dupont_len, 16.3, sr_pcb]) cube([dupont_len - 1, 10, 2.5]);
        color("white") translate([sr_jst[1] - 2.9, sr_jst[0] - 4.95, -sr_jst_h])
            cube([5.8, 9.9, sr_jst_h]);                                      // probe socket
    }
}

module dummy_battery() {
    color("steelblue", 0.8) translate([bat_x0 + clr, bat_y0 + clr, in_h - bat[2]])
        cube(bat);
}

module assembly() {
    color("lightgray") base();
    color("lightgray", 0.35) lid_to_case() lid();
    dummy_firebeetle();
    dummy_jsn();
    dummy_battery();
    color("orange") {
        translate([btn_x, -wall - rod_out, btn_z]) rotate([-90, 0, 0])
            translate([0, 0, rst_rod_len + collar_t]) mirror([0, 0, 1]) push_rod(rst_rod_len);
        translate([btn_x, in_w + wall + rod_out, btn_z]) rotate([90, 0, 0])
            translate([0, 0, boot_rod_len + collar_t]) mirror([0, 0, 1]) push_rod(boot_rod_len);
    }
}

// ---------------------------------------------------------------- output
if (part == "base") base();
else if (part == "lid") lid();
else if (part == "buttons") buttons();
else if (part == "all") {
    base();
    translate([0, out_w + 25, 0]) lid();
    translate([0, -40, 0]) buttons();
}
else assembly();
