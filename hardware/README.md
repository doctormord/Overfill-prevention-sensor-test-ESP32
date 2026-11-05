# Hardware

| File | What it is |
|---|---|
| `gwg-tester.kicad_sch` | schematic, KiCad 8 |
| `gwg-tester.kicad_pro` | project file — open this one |
| `gwg.kicad_sym` | symbols, local to the project |
| `sym-lib-table` | makes KiCad find them |
| `gwg-tester.svg` / `.png` | the same schematic, for reading on screen |
| `BOM.md` | parts, and the traps worth knowing about |
| `generate_schematic.py` | writes all of the above |

Everything here is generated. `generate_schematic.py` holds one placement
table and emits both the KiCad file and the SVG from it, so the drawing in
the documentation and the drawing you open in KiCad cannot drift apart.

```
python3 generate_schematic.py
```

Before writing anything it checks that every pin lands on a wire, that
every pin is on the 1.27 mm grid, that every net label sits on a wire, and
that every junction joins at least three things. Those four checks catch
the failure that matters: a schematic that looks right and is not
connected.

Edit the tables near the top of the script, not the generated files.
