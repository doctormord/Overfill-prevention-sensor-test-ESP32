# Report generator

`index.html` — open it in a browser. That is the whole installation.

Reads a measurement log from the tester over Web Serial, or from a saved
file, and writes an A4 test report as PDF in German or English.

## Why one file with no dependencies

A test report has to be reproducible years after the measurement. A tool
that pulls a charting library and a PDF library from a package registry at
build time is reproducible exactly as long as those registries, those
versions and that build chain keep working. So there is no build step and
nothing is fetched: the PDF writer, the chart code and the log parser are
all in this file, in about 900 lines.

The PDF writer covers the part of PDF 1.4 a technical report needs — the
base-14 fonts with WinAnsi encoding, lines, rectangles, polylines, filled
circles and text. The charts are drawn as vectors straight into the page,
so they stay sharp at any zoom and the file stays around 35 kB.

## Browser support

| Feature | Requirement |
|---|---|
| Open a `.log` file, build a PDF | any current browser |
| Read the tester over Web Serial | Chrome, Edge or Opera |

Web Serial needs a secure context. `file://` counts, so opening the file
from disk works; serving it over plain `http://` from another host does not.

## Input

Both protocol versions are accepted — see
[`../../docs/serial-protocol.md`](../../docs/serial-protocol.md). Metadata
that is absent is reported as `—` rather than guessed, and a quantity that
was not measured is marked as not tested rather than left blank.

## Output

Two A4 pages: unit under test, conditions, a result table with the
verdict, the heating curve, the cooling curve, the method and its
uncertainty, remarks, and a signature block. Section 6 describes the
method the firmware actually implements; if the firmware changes, that
section has to change with it.
