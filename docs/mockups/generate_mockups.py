#!/usr/bin/env python3
"""
Draws the tester's screens as SVG, one sheet per language.

These are mock-ups, not screenshots: they follow the coordinates and the
TFT_eSPI colour constants used in gwg_tester.ino draw(), so the sheet can
be checked against the firmware line by line. The fonts are approximated --
TFT_eSPI's bitmap fonts 2/4/6 have no web equivalent -- so treat the text
metrics as indicative and the layout as exact.

    python3 generate_mockups.py
"""

import os

W, H = 240, 135
SCALE = 2
COLS, GAP = 3, 26
LABEL_H = 20

# TFT_eSPI 16-bit colour constants, converted to 8-bit per channel
C = {
    "BLACK":     "#000000",
    "WHITE":     "#ffffff",
    "RED":       "#ff0000",
    "GREEN":     "#00ff00",
    "BLUE":      "#0000ff",
    "CYAN":      "#00ffff",
    "YELLOW":    "#ffff00",
    "ORANGE":    "#ffb600",
    "PURPLE":    "#7b007b",
    "DARKGREEN": "#007d00",
    "DARKGREY":  "#7b7d7b",
    "LIGHTGREY": "#d2d2d2",
}

# font id -> (px size, baseline offset from the top-left datum)
FONT = {2: (12, 11), 4: (20, 19), 6: (37, 35)}
FAMILY = "DejaVu Sans Mono, SF Mono, Menlo, Consolas, monospace"


class Screen:
    def __init__(self, key, note):
        self.key, self.note = key, note
        self.ops = []

    def fill(self, x, y, w, h, col):
        self.ops.append(('<rect x="%g" y="%g" width="%g" height="%g" '
                         'fill="%s"/>' % (x, y, w, h, C[col])))

    def frame(self, x, y, w, h, col):
        self.ops.append(('<rect x="%g" y="%g" width="%g" height="%g" '
                         'fill="none" stroke="%s" stroke-width="1"/>'
                         % (x + .5, y + .5, w - 1, h - 1, C[col])))

    def txt(self, s, x, y, font, col, anchor="start"):
        size, base = FONT[font]
        s = s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
        self.ops.append('<text x="%g" y="%g" font-family="%s" font-size="%g" '
                        'fill="%s" text-anchor="%s" '
                        'xml:space="preserve">%s</text>'
                        % (x, y + base, FAMILY, size, C[col], anchor, s))

    def header(self, phase, mode, col):
        self.fill(0, 0, 240, 22, col)
        self.txt(phase, 5, 4, 2, "WHITE")
        self.txt(mode, 235, 4, 2, "WHITE", "end")


# --------------------------------------------------------------------------
# the screens, following draw() in gwg_tester.ino
# --------------------------------------------------------------------------
STR = {
    "de": dict(
        idle="BEREIT", heat="AUFHEIZEN", hold="JETZT EINTAUCHEN",
        trip="ABKUEHLEN", cool="ABKUEHLEN (LUFT)", result="ERGEBNIS",
        diag="DIAGNOSE", fault="FEHLER",
        field="FELD", shop="WERKSTATT", mdiag="DIAG",
        node="Knoten A 18.9 V", btn_start="rechts = Start",
        btn_mode="links = Modus wechseln",
        l_heat="Aufheizen", l_shut="Abschalten", l_tau="tau63 Luft",
        rch="R kalt 118  heiss 630 Ohm", cold="kalt nach 2 s",
        target_r="Ziel < 124 Ohm", target_i="Ziel > 46.6 mA",
        pas="BESTANDEN", diag_hint="FET EIN - Taste = Ende",
        fault_hint="Taste = neuer Test", fault_msg="kein GWG / Bruch",
    ),
    "en": dict(
        idle="READY", heat="HEATING", hold="IMMERSE NOW",
        trip="COOLING", cool="COOLING (AIR)", result="RESULT",
        diag="DIAGNOSTICS", fault="FAULT",
        field="FIELD", shop="SHOP", mdiag="DIAG",
        node="node A 18.9 V", btn_start="right = start",
        btn_mode="left = change mode",
        l_heat="Heat-up", l_shut="Shutoff", l_tau="tau63 air",
        rch="R cold 118  hot 630 Ohm", cold="cold after 2 s",
        target_r="target < 124 Ohm", target_i="target > 46.6 mA",
        pas="PASS", diag_hint="switch ON - button = exit",
        fault_hint="button = new test", fault_msg="no probe / open",
    ),
}

NOTE = {
    "de": dict(
        idle="Leerlauf. Schalter aus, Anzeige der Versorgung und von Knoten A.",
        heat="Heizphase. dI zeigt die Steigung, 2/3 die ruhigen Fenster.",
        cool="Luftabkuehlung im Modus FELD. R statt I, weil R hier mehr sagt.",
        resf="Ergebnis FELD. tau63 und Zeit bis zum Kaltwert, kein Gut/Schlecht.",
        hold="Werkstatt. Strom bleibt an, jetzt eintauchen.",
        trip="Abschaltmessung laeuft. Balken gegen 1,5 s.",
        ress="Ergebnis WERKSTATT mit Bewertung.",
        diag="Diagnose. Rohe ADC-Werte, Schalter dauerhaft ein.",
        fault="Fehlerbild. Kein Fuehler angeschlossen.",
    ),
    "en": dict(
        idle="Idle. Switch off, showing the supply and node A.",
        heat="Heating. dI is the slope, 2/3 the quiet windows so far.",
        cool="Air cooling, FIELD mode. R rather than I, because R says more here.",
        resf="FIELD result. tau63 and time back to cold, no pass/fail.",
        hold="Shop mode. Current stays on, immerse the probe now.",
        trip="Shutoff measurement running. Bar runs against 1.5 s.",
        ress="SHOP result with a verdict.",
        diag="Diagnostics. Raw ADC values, switch permanently on.",
        fault="Fault screen. No probe connected.",
    ),
}


def big(s, value, unit, col="WHITE"):
    s.txt(value, 5, 28, 6, col)
    s.txt(unit, 150, 52, 4, "LIGHTGREY")


def build(lang):
    t, n = STR[lang], NOTE[lang]
    out = []

    s = Screen("idle", n["idle"])
    s.header(t["idle"], t["field"], "DARKGREY")
    big(s, "18.9", "V")
    s.txt(t["node"], 5, 80, 2, "LIGHTGREY")
    s.txt(t["btn_start"], 5, 98, 2, "GREEN")
    s.txt(t["btn_mode"], 5, 116, 2, "YELLOW")
    out.append(s)

    s = Screen("heat", n["heat"])
    s.header(t["heat"], t["field"], "ORANGE")
    big(s, "31.4", "mA")
    s.txt("R 402 Ohm", 5, 84, 2, "LIGHTGREY")
    s.txt("U 12.6 V", 125, 84, 2, "LIGHTGREY")
    s.txt("t = 14.20 s", 5, 106, 2, "WHITE")
    s.txt("dI -1.84  0/3", 130, 106, 2, "LIGHTGREY")
    s.frame(5, 128, 230, 6, "DARKGREY")
    s.fill(5, 128, 73, 6, "GREEN")
    out.append(s)

    s = Screen("cool", n["cool"])
    s.header(t["cool"], t["field"], "BLUE")
    big(s, "241", "Ohm")
    s.txt(t["target_r"], 5, 84, 2, "LIGHTGREY")
    s.txt("t = 0.10 s", 5, 106, 2, "WHITE")
    s.txt("tau63 = 0.06 s", 125, 106, 2, "CYAN")
    out.append(s)

    s = Screen("result_field", n["resf"])
    s.header(t["result"], t["field"], "DARKGREEN")
    s.txt(t["l_heat"], 5, 30, 2, "WHITE")
    s.txt("26.2 s", 115, 28, 4, "GREEN")
    s.txt(t["l_tau"], 5, 62, 2, "WHITE")
    s.txt("0.06 s", 115, 60, 4, "CYAN")
    s.txt(t["cold"], 5, 92, 2, "LIGHTGREY")
    s.txt(t["rch"], 5, 112, 2, "LIGHTGREY")
    out.append(s)

    s = Screen("hold", n["hold"])
    s.header(t["hold"], t["shop"], "CYAN")
    big(s, "24.1", "mA")
    s.txt("R 630 Ohm", 5, 84, 2, "LIGHTGREY")
    s.txt("U 15.2 V", 125, 84, 2, "LIGHTGREY")
    s.txt("t = 3.04 s", 5, 106, 2, "WHITE")
    s.txt(t["target_i"], 125, 106, 2, "YELLOW")
    out.append(s)

    s = Screen("trip", n["trip"])
    s.header(t["trip"], t["shop"], "BLUE")
    big(s, "38.7", "mA")
    s.txt("R 302 Ohm", 5, 84, 2, "LIGHTGREY")
    s.txt("U 11.7 V", 125, 84, 2, "LIGHTGREY")
    s.txt("t = 0.62 s", 5, 106, 2, "WHITE")
    s.txt(t["target_i"], 125, 106, 2, "YELLOW")
    s.frame(5, 128, 230, 6, "DARKGREY")
    s.fill(5, 128, 95, 6, "GREEN")
    out.append(s)

    s = Screen("result_shop", n["ress"])
    s.header(t["result"], t["shop"], "DARKGREEN")
    s.txt(t["l_heat"], 5, 30, 2, "WHITE")
    s.txt("26.2 s", 115, 28, 4, "GREEN")
    s.txt(t["l_shut"], 5, 62, 2, "WHITE")
    s.txt("0.84 s", 115, 60, 4, "GREEN")
    s.txt(t["rch"], 5, 96, 2, "LIGHTGREY")
    s.txt(t["pas"], 5, 114, 2, "GREEN")
    out.append(s)

    s = Screen("diag", n["diag"])
    s.header(t["diag"], t["mdiag"], "PURPLE")
    s.txt("ADC A     1732 mV", 5, 26, 2, "WHITE")
    s.txt("ADC Sup   1741 mV", 5, 44, 2, "WHITE")
    s.txt("U_A 12.74 V   Vsup 18.91 V", 5, 66, 2, "YELLOW")
    s.txt("I  39.15 mA   R     325 Ohm", 5, 88, 2, "CYAN")
    s.txt(t["diag_hint"], 5, 114, 2, "LIGHTGREY")
    out.append(s)

    s = Screen("fault", n["fault"])
    s.header(t["fault"], t["field"], "RED")
    s.txt(t["fault_msg"], 5, 40, 4, "RED")
    s.txt("R = 99100 Ohm   I = 0.2 mA", 5, 80, 2, "LIGHTGREY")
    s.txt(t["fault_hint"], 5, 114, 2, "LIGHTGREY")
    out.append(s)

    return out


def sheet(screens, lang, path):
    rows = (len(screens) + COLS - 1) // COLS
    cw = W * SCALE + GAP
    ch = H * SCALE + GAP + LABEL_H + 16
    width = COLS * cw + GAP
    height = rows * ch + GAP + 34

    o = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" '
         f'height="{height}" viewBox="0 0 {width} {height}">',
         f'<rect width="{width}" height="{height}" fill="#f4f1ea"/>',
         f'<text x="{GAP}" y="30" font-family="Inter,system-ui,sans-serif" '
         f'font-size="17" font-weight="600" fill="#1b2a33">'
         f'TTGO T-Display 240 x 135 &#8212; '
         f'{"Anzeigen" if lang == "de" else "screens"} ({lang.upper()})</text>']

    for i, s in enumerate(screens):
        cx = GAP + (i % COLS) * cw
        cy = 48 + (i // COLS) * ch
        o.append(f'<rect x="{cx-4}" y="{cy-4}" width="{W*SCALE+8}" '
                 f'height="{H*SCALE+8}" rx="7" fill="#2a2f33"/>')
        o.append(f'<g transform="translate({cx},{cy}) scale({SCALE})">')
        o.append(f'<rect width="{W}" height="{H}" fill="#000000"/>')
        o.extend(s.ops)
        o.append('</g>')
        ty = cy + H * SCALE + 18
        o.append(f'<text x="{cx}" y="{ty}" font-family="Inter,system-ui,'
                 f'sans-serif" font-size="12" font-weight="600" '
                 f'fill="#1b2a33">{s.key}</text>')
        note = s.note.replace("&", "&amp;").replace("<", "&lt;")
        o.append(f'<text x="{cx}" y="{ty+15}" font-family="Inter,system-ui,'
                 f'sans-serif" font-size="11" fill="#5c6b74">{note}</text>')

    o.append('</svg>')
    with open(path, "w") as f:
        f.write("\n".join(o) + "\n")


if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    for lang in ("de", "en"):
        sheet(build(lang), lang, os.path.join(here, f"screens-{lang}.svg"))
    print("wrote screens-de.svg, screens-en.svg")
