#!/usr/bin/env python3
"""
Generates hardware/gwg-tester.kicad_sch, hardware/gwg.kicad_sym and
hardware/gwg-tester.svg from one description of the circuit.

Why generated: the schematic and the SVG in the documentation must not be
able to drift apart. Both come out of the placement table below, so a wire
that moves moves in both.

Coordinates are given in grid units of 1.27 mm. KiCad symbol bodies use
Y-up, KiCad schematics use Y-down; the two horizontal/vertical variants of
each passive exist so that nothing in here depends on rotation conventions.

    python3 generate_schematic.py
"""

import hashlib
import datetime

U = 1.27                      # grid unit, mm
PROJECT = "gwg-tester"
TITLE = "Grenzwertgeber (PTC) test set - analogue front end"
REV = "1.0"
DATE = "2026-11-04"
COMPANY = "github.com/doctormord/Overfill-prevention-sensor-test-ESP32"

_n = [0]


def uid(tag=""):
    """Deterministic UUIDs: regenerating must not produce a noisy diff."""
    _n[0] += 1
    h = hashlib.sha1(f"{PROJECT}:{tag}:{_n[0]}".encode()).hexdigest()
    return f"{h[0:8]}-{h[8:12]}-{h[12:16]}-{h[16:20]}-{h[20:32]}"


ROOT_UUID = uid("root")


def mm(v):
    return round(v * U, 4)


# --------------------------------------------------------------------------
# symbol library
# --------------------------------------------------------------------------
# pins: (number, name, etype, (x, y) in symbol coords / mm, direction)
# direction is only used for the stub the SVG draws.

SYMBOLS = {
    "R_V": dict(  # vertical resistor, IEC body
        ref="R", kind="passive",
        body=[("rect", -1.016, -2.54, 1.016, 2.54)],
        pins=[("1", "~", "passive", 0.0, 3.81, "up"),
              ("2", "~", "passive", 0.0, -3.81, "down")],
        ref_at=(2.54, 1.27), val_at=(2.54, -1.27), just="left",
    ),
    "R_H": dict(  # horizontal resistor
        ref="R", kind="passive",
        body=[("rect", -2.54, -1.016, 2.54, 1.016)],
        pins=[("1", "~", "passive", -3.81, 0.0, "left"),
              ("2", "~", "passive", 3.81, 0.0, "right")],
        ref_at=(0.0, 2.794), val_at=(0.0, -2.794), just="center",
    ),
    "C_V": dict(  # vertical unpolarised capacitor
        ref="C", kind="passive",
        body=[("line", -1.778, 0.508, 1.778, 0.508),
              ("line", -1.778, -0.508, 1.778, -0.508)],
        pins=[("1", "~", "passive", 0.0, 3.81, "up"),
              ("2", "~", "passive", 0.0, -3.81, "down")],
        ref_at=(2.54, 1.27), val_at=(2.54, -1.27), just="left",
    ),
    "CONN_2R": dict(  # 2-pin terminal, pins to the right
        ref="J", kind="conn",
        body=[("rect", -5.08, -3.81, 0.0, 3.81)],
        pins=[("1", "1", "passive", 5.08, 2.54, "right"),
              ("2", "2", "passive", 5.08, -2.54, "right")],
        ref_at=(-2.54, 5.08), val_at=(-2.54, -5.08), just="center",
        pin_names=True,
    ),
    "CONN_2L": dict(  # 2-pin terminal, pins to the left
        ref="J", kind="conn",
        body=[("rect", 0.0, -3.81, 5.08, 3.81)],
        pins=[("1", "1", "passive", -5.08, 2.54, "left"),
              ("2", "2", "passive", -5.08, -2.54, "left")],
        ref_at=(2.54, 5.08), val_at=(2.54, -5.08), just="center",
        pin_names=True,
    ),
    "SSR_4": dict(  # CPC1906Y: LED input left, isolated output right
        ref="U", kind="ic",
        body=[("rect", -7.62, -6.35, 7.62, 6.35),
              ("line", 0.0, -6.35, 0.0, 6.35)],
        pins=[("1", "A", "input", -10.16, 2.54, "left"),
              ("2", "K", "input", -10.16, -5.08, "left"),
              ("3", "S1", "passive", 10.16, 2.54, "right"),
              ("4", "S2", "passive", 10.16, -5.08, "right")],
        ref_at=(0.0, 8.89), val_at=(0.0, -8.89), just="center",
        pin_names=True, pin_numbers=True,
    ),
    "MODULE_4": dict(  # TTGO T-Display, only the pins this circuit uses
        ref="MOD", kind="ic",
        body=[("rect", -12.7, -8.89, 7.62, 8.89)],
        pins=[("1", "IO25", "output", 10.16, 5.08, "right"),
              ("2", "IO32", "input", 10.16, 1.27, "right"),
              ("3", "IO33", "input", 10.16, -2.54, "right"),
              ("4", "GND", "power_in", 10.16, -6.35, "right")],
        ref_at=(-12.7, 11.43), val_at=(-12.7, -11.43), just="left",
        pin_names=True, pin_numbers=True,
    ),
    "PWR_19V": dict(
        ref="#PWR", kind="power",
        body=[("line", 0.0, 0.0, 0.0, 1.905),
              ("line", -1.27, 1.905, 1.27, 1.905)],
        pins=[("1", "+19V", "power_in", 0.0, 0.0, "up")],
        ref_at=(0.0, -2.54), val_at=(0.0, 3.81), just="center",
        power=True,
    ),
    "GND": dict(
        ref="#PWR", kind="power",
        body=[("line", 0.0, 0.0, 0.0, -1.905),
              ("line", -1.905, -1.905, 1.905, -1.905),
              ("line", -1.143, -2.667, 1.143, -2.667),
              ("line", -0.381, -3.429, 0.381, -3.429)],
        pins=[("1", "GND", "power_in", 0.0, 0.0, "down")],
        ref_at=(0.0, 2.54), val_at=(0.0, -5.08), just="center",
        power=True,
    ),
}

# --------------------------------------------------------------------------
# placement: (ref, symbol, value, gx, gy, footprint, dnp)
# --------------------------------------------------------------------------
PARTS = [
    ("J1",   "CONN_2R",  "19V DC in",   24, 26, "", False),
    ("PWR1", "PWR_19V",  "+19V",        34, 24, "", False),
    ("GND1", "GND",      "GND",         34, 28, "", False),

    ("PWR2", "PWR_19V",  "+19V",        56, 22, "", False),
    ("R1",   "R_V",      "159R",        56, 33, "see note 1", False),

    ("R2",   "R_V",      "1M",          70, 43, "", False),
    ("J2",   "CONN_2L",  "probe",       86, 42, "", False),

    ("R3",   "R_H",      "220R",       104, 52, "", False),
    ("U1",   "SSR_4",    "CPC1906Y",   120, 54, "", False),
    ("GND2", "GND",      "GND",        112, 64, "", False),
    ("GND3", "GND",      "GND",        128, 64, "", False),

    ("R4",   "R_V",      "100k",       150, 33, "", False),
    ("R5",   "R_V",      "10k",        150, 43, "", False),
    ("C1",   "C_V",      "10n",        158, 41, "", True),
    ("GND4", "GND",      "GND",        150, 52, "", False),
    ("GND5", "GND",      "GND",        158, 52, "", False),

    ("PWR3", "PWR_19V",  "+19V",       170, 24, "", False),
    ("R6",   "R_V",      "100k",       170, 33, "", False),
    ("R7",   "R_V",      "10k",        170, 43, "", False),
    ("C2",   "C_V",      "10n",        178, 41, "", True),
    ("GND6", "GND",      "GND",        170, 52, "", False),
    ("GND7", "GND",      "GND",        178, 52, "", False),

    ("MOD1", "MODULE_4", "TTGO T-Display", 44, 80, "", False),
    ("GND8", "GND",      "GND",         52, 91, "", False),
]

# wires: list of polylines in grid units
WIRES = [
    [(28, 24), (34, 24)],                       # J1.1 -> +19V
    [(28, 28), (34, 28)],                       # J1.2 -> GND
    [(56, 22), (56, 30)],                       # +19V -> R1.1
    [(56, 36), (56, 40)],                       # R1.2 -> NODE_A
    [(56, 40), (82, 40)],                       # NODE_A rail to probe
    [(70, 46), (70, 50)],                       # R2.2 -> SW
    [(70, 50), (82, 50)],                       # SW rail
    [(82, 44), (82, 50)],                       # J2.2 -> SW
    [(96, 52), (101, 52)],                      # GATE -> R3.1
    [(107, 52), (112, 52)],                     # R3.2 -> U1.1 (LED anode)
    [(112, 58), (112, 64)],                     # U1.2 (cathode) -> GND
    [(128, 52), (134, 52)],                     # U1.3 -> SW
    [(128, 58), (128, 64)],                     # U1.4 -> GND
    [(150, 26), (150, 30)],                     # NODE_A -> R4.1
    [(150, 36), (150, 38)],                     # R4.2 -> tap
    [(150, 38), (150, 40)],                     # tap -> R5.1
    [(150, 46), (150, 52)],                     # R5.2 -> GND
    [(150, 38), (158, 38)],                     # ADC_A tap -> C1.1
    [(158, 44), (158, 52)],                     # C1.2 -> GND
    [(170, 24), (170, 30)],                     # +19V -> R6.1
    [(170, 36), (170, 38)],
    [(170, 38), (170, 40)],
    [(170, 46), (170, 52)],
    [(170, 38), (178, 38)],                     # ADC_SUP tap -> C2.1
    [(178, 44), (178, 52)],                     # C2.2 -> GND
    [(52, 76), (60, 76)],                       # MOD1 IO25 -> GATE
    [(52, 79), (60, 79)],                       # MOD1 IO32 -> ADC_A
    [(52, 82), (60, 82)],                       # MOD1 IO33 -> ADC_SUP
    [(52, 85), (52, 91)],                       # MOD1 GND
]

# labels: (text, gx, gy, anchor)  anchor: left | right
LABELS = [
    ("NODE_A",   60, 40, "left"),
    ("SW",       76, 50, "left"),
    ("GATE",     96, 52, "right"),
    ("SW",      134, 52, "left"),
    ("NODE_A",  150, 26, "right"),
    ("ADC_A",   154, 38, "left"),
    ("ADC_SUP", 174, 38, "left"),
    ("GATE",     60, 76, "left"),
    ("ADC_A",    60, 79, "left"),
    ("ADC_SUP",  60, 82, "left"),
]

JUNCTIONS = [(70, 40), (150, 38), (170, 38)]

NOTES = [
    (104, 72, "Note 1  R1 = 159.1 R, measured -- not read off a colour code and not"),
    (104, 75, "        back-calculated from the two ADC channels. Build it from standard"),
    (104, 78, "        values and measure the result:"),
    (104, 81, "            6 x 1k   parallel = 166.7 R   0.10 W each"),
    (104, 84, "            3 x 470R parallel = 156.7 R   0.21 W each"),
    (104, 87, "        Enter the measured value in firmware/gwg_tester/config.h as R_SERIES."),
    (104, 93, "Note 2  C1 and C2 are DNP. If fitted, fit BOTH with the same value: the"),
    (104, 96, "        current is the difference of the two channels, so asymmetric"),
    (104, 99, "        filtering feeds the full supply ripple into the current reading"),
    (104, 102, "        instead of cancelling it. A capacitor on node A alone also stretches"),
    (104, 105, "        the settle time and blocks the pulsed measurement while cooling."),
    (104, 111, "Note 3  Tie the 19 V supply ground to module ground. Power MOD1 from USB-C,"),
    (104, 114, "        not from the 19 V rail."),
    (104, 120, "Note 4  R2 keeps a defined current flowing with no probe fitted, so"),
    (104, 123, "        open-circuit detection cannot read a plausible value off a floating"),
    (104, 126, "        node. It is divided out in firmware: R_DIV_A = 110k || 1M = 99.1k."),
    (104, 132, "Note 5  GPIO32/33 are on ADC1. ADC2 is unusable while WiFi is active."),
    (104, 135, "        GPIO0 and GPIO35 are the on-board buttons, no external wiring."),
    (104, 141, "WARNING Not intrinsically safe. Do not test in fuel oil or a hazardous area."),
]


# --------------------------------------------------------------------------
# KiCad symbol library
# --------------------------------------------------------------------------
def sym_body(name, s):
    out = []
    for g in s["body"]:
        if g[0] == "rect":
            out.append(
                f'      (rectangle (start {g[1]} {g[2]}) (end {g[3]} {g[4]})\n'
                f'        (stroke (width 0.254) (type default)) (fill (type none)))')
        else:
            out.append(
                f'      (polyline (pts (xy {g[1]} {g[2]}) (xy {g[3]} {g[4]}))\n'
                f'        (stroke (width 0.254) (type default)) (fill (type none)))')
    return "\n".join(out)


def sym_pins(name, s):
    out = []
    for num, pname, etype, px, py, _d in s["pins"]:
        # point the pin stub away from the body
        if abs(px) >= abs(py):
            ang = 180 if px > 0 else 0
        else:
            ang = 270 if py > 0 else 90
        out.append(
            f'      (pin {etype} line (at {px} {py} {ang}) (length 2.54)\n'
            f'        (name "{pname}" (effects (font (size 1.27 1.27))))\n'
            f'        (number "{num}" (effects (font (size 1.27 1.27)))))')
    return "\n".join(out)


def symbol_def(name, s):
    hide_names = "" if s.get("pin_names") else " hide"
    hide_nums = "" if s.get("pin_numbers") else " hide"
    power = "\n    (power)" if s.get("power") else ""
    rx, ry = s["ref_at"]
    vx, vy = s["val_at"]
    just = s["just"]
    jf = "" if just == "center" else f" (justify {just})"
    return f'''  (symbol "{name}"
    (pin_numbers{hide_nums})
    (pin_names (offset 0.762){hide_names})
    (exclude_from_sim no) (in_bom yes) (on_board yes){power}
    (property "Reference" "{s['ref']}" (at {rx} {ry} 0)
      (effects (font (size 1.27 1.27)){jf}))
    (property "Value" "{name}" (at {vx} {vy} 0)
      (effects (font (size 1.27 1.27)){jf}))
    (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
    (property "Datasheet" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
    (property "Description" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
    (symbol "{name}_0_1"
{sym_body(name, s)}
    )
    (symbol "{name}_1_1"
{sym_pins(name, s)}
    )
  )'''


def write_symlib(path):
    body = "\n".join(symbol_def(n, s) for n, s in SYMBOLS.items())
    with open(path, "w") as f:
        f.write('(kicad_symbol_lib\n  (version 20231120)\n'
                '  (generator "generate_schematic.py")\n'
                '  (generator_version "8.0")\n')
        f.write(body)
        f.write("\n)\n")


# --------------------------------------------------------------------------
# schematic
# --------------------------------------------------------------------------
def pin_xy(part, number):
    """Schematic position of a pin. Symbol Y is up, schematic Y is down."""
    ref, name, val, gx, gy, fp, dnp = part
    s = SYMBOLS[name]
    for num, _n2, _e, px, py, _d in s["pins"]:
        if num == number:
            return (gx * U + px, gy * U - py)
    raise KeyError(f"{ref} has no pin {number}")


def place_symbol(part):
    ref, name, val, gx, gy, fp, dnp = part
    s = SYMBOLS[name]
    x, y = mm(gx), mm(gy)
    rx, ry = s["ref_at"]
    vx, vy = s["val_at"]
    just = s["just"]
    jf = "" if just == "center" else f" (justify {just})"
    hide_ref = " hide" if s.get("power") else ""
    pins = "\n".join(f'    (pin "{p[0]}" (uuid "{uid(ref + p[0])}"))'
                     for p in s["pins"])
    props = [
        f'    (property "Reference" "{ref}" (at {round(x + rx, 4)} {round(y - ry, 4)} 0)\n'
        f'      (effects (font (size 1.27 1.27)){jf}{hide_ref}))',
        f'    (property "Value" "{val}" (at {round(x + vx, 4)} {round(y - vy, 4)} 0)\n'
        f'      (effects (font (size 1.27 1.27)){jf}))',
        f'    (property "Footprint" "{fp}" (at {x} {y} 0)\n'
        f'      (effects (font (size 1.27 1.27)) hide))',
    ]
    return f'''  (symbol
    (lib_id "gwg:{name}") (at {x} {y} 0) (unit 1)
    (exclude_from_sim no) (in_bom yes) (on_board yes) (dnp {"yes" if dnp else "no"})
    (uuid "{uid(ref)}")
{chr(10).join(props)}
{pins}
    (instances
      (project "{PROJECT}"
        (path "/{ROOT_UUID}" (reference "{ref}") (unit 1))))
  )'''


def write_sch(path):
    out = ['(kicad_sch',
           '  (version 20231120)',
           '  (generator "generate_schematic.py")',
           '  (generator_version "8.0")',
           f'  (uuid "{ROOT_UUID}")',
           '  (paper "A4")',
           '  (title_block',
           f'    (title "{TITLE}")',
           f'    (date "{DATE}")',
           f'    (rev "{REV}")',
           f'    (company "{COMPANY}")',
           '    (comment 1 "Overfill prevention sensor (Kaltleiter) test set")',
           '    (comment 2 "Low-side switching, current by back-calculation")',
           '  )',
           '  (lib_symbols']
    for n, s in SYMBOLS.items():
        out.append(symbol_def(n, s))
    out.append('  )')

    for poly in WIRES:
        for a, b in zip(poly, poly[1:]):
            if a == b:
                continue
            out.append(f'  (wire (pts (xy {mm(a[0])} {mm(a[1])}) '
                       f'(xy {mm(b[0])} {mm(b[1])}))\n'
                       f'    (stroke (width 0) (type default)) (uuid "{uid("w")}"))')

    for gx, gy in JUNCTIONS:
        out.append(f'  (junction (at {mm(gx)} {mm(gy)}) (diameter 0) '
                   f'(color 0 0 0 0) (uuid "{uid("j")}"))')

    for text, gx, gy, anchor in LABELS:
        j = "left" if anchor == "left" else "right"
        out.append(f'  (label "{text}" (at {mm(gx)} {mm(gy)} 0)\n'
                   f'    (effects (font (size 1.27 1.27)) (justify {j} bottom))\n'
                   f'    (uuid "{uid("l")}"))')

    for gx, gy, text in NOTES:
        out.append(f'  (text "{text}" (exclude_from_sim yes) '
                   f'(at {mm(gx)} {mm(gy)} 0)\n'
                   f'    (effects (font (size 1.27 1.27) (face "Monospace")) '
                   f'(justify left bottom))\n'
                   f'    (uuid "{uid("t")}"))')

    for p in PARTS:
        out.append(place_symbol(p))

    out.append('  (sheet_instances (path "/" (page "1")))')
    out.append(')')
    with open(path, "w") as f:
        f.write("\n".join(out) + "\n")


def write_pro(path):
    with open(path, "w") as f:
        f.write('{\n  "board": {},\n  "libraries": {"pinned_footprint_libs": [], '
                '"pinned_symbol_libs": []},\n'
                '  "meta": {"filename": "' + PROJECT + '.kicad_pro", "version": 1},\n'
                '  "schematic": {"legacy_lib_dir": "", "legacy_lib_list": []},\n'
                '  "sheets": [["' + ROOT_UUID + '", "Root"]],\n'
                '  "text_variables": {}\n}\n')


def write_symlibtable(path):
    with open(path, "w") as f:
        f.write('(sym_lib_table\n  (version 7)\n'
                '  (lib (name "gwg")(type "KiCad")(uri "${KIPRJMOD}/gwg.kicad_sym")'
                '(options "")(descr "GWG tester local symbols"))\n)\n')


# --------------------------------------------------------------------------
# SVG, drawn from the same tables
# --------------------------------------------------------------------------
SVG_SCALE = 3.2
PAGE = (297.0, 210.0)


def svg_header():
    w, h = PAGE[0] * SVG_SCALE, PAGE[1] * SVG_SCALE
    return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{w:.0f}" '
            f'height="{h:.0f}" viewBox="0 0 {PAGE[0]} {PAGE[1]}">\n'
            f'<rect width="{PAGE[0]}" height="{PAGE[1]}" fill="#fffef9"/>\n'
            f'<g stroke="#1b2a33" stroke-width="0.25" fill="none" '
            f'stroke-linecap="round">\n')


def svg_text(x, y, s, size=1.5, anchor="start", fill="#1b2a33",
             family="DejaVu Sans, Helvetica, sans-serif", weight="normal"):
    s = (s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;"))
    return (f'<text x="{x:.3f}" y="{y:.3f}" font-family="{family}" '
            f'font-size="{size}" font-weight="{weight}" text-anchor="{anchor}" '
            f'fill="{fill}" stroke="none">{s}</text>\n')


def write_svg(path):
    o = [svg_header()]

    # frame and title block
    o.append(f'<rect x="5" y="5" width="{PAGE[0]-10}" height="{PAGE[1]-10}" '
             f'stroke="#1b2a33" stroke-width="0.4"/>\n')
    tb_w, tb_h = 160.0, 22.0
    tx, ty = PAGE[0] - 5 - tb_w, PAGE[1] - 5 - tb_h
    o.append(f'<rect x="{tx}" y="{ty}" width="{tb_w}" height="{tb_h}" '
             f'stroke="#1b2a33" stroke-width="0.4"/>\n')
    o.append(f'<line x1="{tx}" y1="{ty+8}" x2="{tx+tb_w}" y2="{ty+8}" '
             f'stroke="#1b2a33" stroke-width="0.25"/>\n')
    o.append(svg_text(tx + 3, ty + 5.6, TITLE, 3.0, weight="bold"))
    o.append(svg_text(tx + 3, ty + 13, COMPANY, 2.0, fill="#4a5a63"))
    o.append(svg_text(tx + 3, ty + 18.5, f"Rev {REV}  |  {DATE}  |  Sheet 1/1  |  A4",
                      2.0, fill="#4a5a63"))

    # wires
    for poly in WIRES:
        for a, b in zip(poly, poly[1:]):
            if a == b:
                continue
            o.append(f'<line x1="{mm(a[0])}" y1="{mm(a[1])}" '
                     f'x2="{mm(b[0])}" y2="{mm(b[1])}" stroke-width="0.3"/>\n')

    # junctions
    for gx, gy in JUNCTIONS:
        o.append(f'<circle cx="{mm(gx)}" cy="{mm(gy)}" r="0.6" '
                 f'fill="#1b2a33" stroke="none"/>\n')

    # symbols
    for part in PARTS:
        ref, name, val, gx, gy, fp, dnp = part
        s = SYMBOLS[name]
        ox, oy = mm(gx), mm(gy)
        for g in s["body"]:
            if g[0] == "rect":
                x0, y0 = ox + g[1], oy - g[2]
                x1, y1 = ox + g[3], oy - g[4]
                o.append(f'<rect x="{min(x0,x1):.3f}" y="{min(y0,y1):.3f}" '
                         f'width="{abs(x1-x0):.3f}" height="{abs(y1-y0):.3f}" '
                         f'stroke-width="0.3" fill="#ffffff"/>\n')
            else:
                o.append(f'<line x1="{ox+g[1]:.3f}" y1="{oy-g[2]:.3f}" '
                         f'x2="{ox+g[3]:.3f}" y2="{oy-g[4]:.3f}" '
                         f'stroke-width="0.3"/>\n')
        # pin stubs, drawn from body edge to the connection point
        for num, pname, _e, px, py, d in s["pins"]:
            cx, cy = ox + px, oy - py
            dx, dy = {"up": (0, 1), "down": (0, -1),
                      "left": (1, 0), "right": (-1, 0)}[d]
            o.append(f'<line x1="{cx:.3f}" y1="{cy:.3f}" '
                     f'x2="{cx+dx*2.54:.3f}" y2="{cy+dy*2.54:.3f}" '
                     f'stroke-width="0.3"/>\n')
            if s.get("pin_numbers"):
                o.append(svg_text(cx - dx * 1.3, cy - 0.7, num, 1.2,
                                  "end" if d == "right" else "start", "#8a6b2b"))
            if s.get("pin_names"):
                nx = cx + dx * 3.6
                anc = {"up": "middle", "down": "middle",
                       "left": "start", "right": "end"}[d]
                o.append(svg_text(nx, cy + 0.5, pname, 1.4, anc, "#4a5a63"))
        rx, ry = s["ref_at"]
        vx, vy = s["val_at"]
        anc = {"left": "start", "right": "end", "center": "middle"}[s["just"]]
        if not s.get("power"):
            o.append(svg_text(ox + rx, oy - ry, ref, 1.8, anc, weight="bold"))
        o.append(svg_text(ox + vx, oy - vy,
                          val + ("  (DNP)" if dnp else ""), 1.6, anc, "#1b2a33"))

    # labels
    for text, gx, gy, anchor in LABELS:
        x, y = mm(gx), mm(gy)
        anc = "start" if anchor == "left" else "end"
        o.append(svg_text(x + (0.8 if anchor == "left" else -0.8), y - 0.6,
                          text, 1.6, anc, "#0a6a7a", weight="bold"))

    # notes
    for gx, gy, text in NOTES:
        bold = text.startswith("WARNING")
        o.append(svg_text(mm(gx), mm(gy), text, 2.0, "start",
                          "#8a2b12" if bold else "#2d3f49",
                          family="DejaVu Sans Mono, monospace",
                          weight="bold" if bold else "normal"))

    o.append('</g>\n</svg>\n')
    with open(path, "w") as f:
        f.write("".join(o))


# --------------------------------------------------------------------------
# consistency checks -- these are the reason this file exists
# --------------------------------------------------------------------------
def on_segment(pt):
    gx, gy = pt
    for poly in WIRES:
        for a, b in zip(poly, poly[1:]):
            if a == b:
                continue
            if (min(a[0], b[0]) - 1e-9 <= gx <= max(a[0], b[0]) + 1e-9 and
                    min(a[1], b[1]) - 1e-9 <= gy <= max(a[1], b[1]) + 1e-9):
                return True
    return False


def check():
    errors = []
    wire_ends = set()
    for poly in WIRES:
        for a, b in zip(poly, poly[1:]):
            if a != b:
                wire_ends.add(a)
                wire_ends.add(b)
    wire_pts = set()
    for poly in WIRES:
        for p in poly:
            wire_pts.add(p)

    # every non-power pin must land on a wire end or a junction
    for part in PARTS:
        ref, name, val, gx, gy, fp, dnp = part
        for num, pname, _e, px, py, _d in SYMBOLS[name]["pins"]:
            gxp = (gx * U + px) / U
            gyp = (gy * U - py) / U
            key = (round(gxp), round(gyp))
            if abs(gxp - key[0]) > 1e-6 or abs(gyp - key[1]) > 1e-6:
                errors.append(f"{ref}.{num} is off grid: {gxp:.3f},{gyp:.3f}")
            # a pin may land on a wire end OR anywhere along a segment --
            # KiCad connects both, and a T-piece under a pin is the normal
            # way to tap a rail
            if key not in wire_pts and not on_segment(key):
                errors.append(f"{ref}.{num} ({pname}) at {key} touches no wire")

    # labels must sit on a wire
    for text, gx, gy, _a in LABELS:
        on = any(
            (min(a[0], b[0]) - 1e-9 <= gx <= max(a[0], b[0]) + 1e-9 and
             min(a[1], b[1]) - 1e-9 <= gy <= max(a[1], b[1]) + 1e-9)
            for poly in WIRES for a, b in zip(poly, poly[1:]) if a != b)
        if not on:
            errors.append(f'label "{text}" at {gx},{gy} sits on no wire')

    # junctions must be where three or more wire ends or crossings meet
    for gx, gy in JUNCTIONS:
        touch = 0
        for poly in WIRES:
            for a, b in zip(poly, poly[1:]):
                if a == b:
                    continue
                if (min(a[0], b[0]) <= gx <= max(a[0], b[0]) and
                        min(a[1], b[1]) <= gy <= max(a[1], b[1])):
                    # a wire passing straight through counts as two
                    # connections, one that ends here as one
                    touch += 1 if (gx, gy) in (a, b) else 2
        pinhit = sum(
            1 for part in PARTS
            for num, _pn, _e, px, py, _d in SYMBOLS[part[1]]["pins"]
            if (round((part[3] * U + px) / U), round((part[4] * U - py) / U)) == (gx, gy))
        if touch + pinhit < 3:
            errors.append(f"junction at {gx},{gy} joins only {touch + pinhit} things")

    return errors


if __name__ == "__main__":
    import os
    here = os.path.dirname(os.path.abspath(__file__))
    errs = check()
    for e in errs:
        print("CHECK:", e)
    write_symlib(os.path.join(here, "gwg.kicad_sym"))
    write_sch(os.path.join(here, f"{PROJECT}.kicad_sch"))
    write_pro(os.path.join(here, f"{PROJECT}.kicad_pro"))
    write_symlibtable(os.path.join(here, "sym-lib-table"))
    write_svg(os.path.join(here, f"{PROJECT}.svg"))
    print(f"wrote 5 files, {len(errs)} check failures")
