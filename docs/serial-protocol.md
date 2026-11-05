# Serial protocol

115200 baud, 8N1, LF line endings.

Three kinds of line, told apart by their first characters. Anything that
does not match one of them should be ignored by a reader rather than
treated as an error — boot messages from the ESP32 ROM appear before the
firmware prints anything.

| Prefix | Meaning | Translated? |
|---|---|---|
| `# ` | machine-readable metadata, `key=value` | no |
| `## ` | prose for the operator | yes |
| digit | CSV data row | — |

The split exists so that a log recorded on a German unit and a log
recorded on an English unit parse identically. Only the `##` lines change
with `GWG_LANG`; keys, phase tokens and mode tokens never do.

## Data rows

```
t_ms;phase;U_A_mV;Vsup_mV;I_mA;R_ptc_Ohm
```

`t_ms` is milliseconds since the start of the current phase, not since
power-up. `phase` is one of `IDLE HEAT HOLD TRIP COOL RESULT FAULT DIAG`.
Rows are emitted at most every 20 ms during HEAT, HOLD and TRIP, and once
per measuring pulse during COOL.

Each row carries its own redundancy: `I_mA` and `R_ptc_Ohm` can both be
recomputed from `U_A_mV`, `Vsup_mV` and the two resistor values in the
header, so a transcription error shows up as an inconsistency rather than
as a plausible wrong number.

## Metadata keys

Keys appear when the corresponding value becomes known, so the position of
a key in the stream carries information: `t_heat_ms` cannot appear before
the heating phase has ended.

| Key | Unit | When |
|---|---|---|
| `gwg-tester <version>` | — | boot (bare, no `=`) |
| `proto` | — | boot, currently `2` |
| `lang` | — | boot, `de` or `en` |
| `r_series_ohm` | Ω | boot |
| `r_div_a_ohm` | Ω | boot |
| `columns` | — | boot |
| `gain_a` | — | after auto-zero |
| `mode` | — | at start: `FIELD`, `SHOP`, `DIAG` |
| `vsup_v` | V | at start |
| `r_cold_ohm` | Ω | at start |
| `i_cold_ma` | mA | at start |
| `t_heat_ms` | ms | end of heating |
| `i_hot_ma` | mA | end of heating |
| `r_hot_ohm` | Ω | end of heating |
| `i_trip_ma` | mA | end of heating |
| `r63_ohm` | Ω | FIELD, start of cooling |
| `pulse_on_ms` | ms | FIELD, first cooling pulse |
| `pulse_period_ms` | ms | FIELD, first cooling pulse |
| `duty` | — | FIELD, first cooling pulse |
| `duty_warn` | — | only if duty > 8 % |
| `tau63_ms` | ms | FIELD, when resolved |
| `tau63_valid` | — | `1` resolved, `0` bound only |
| `tau63_bound_ms` | ms | FIELD, when unresolved |
| `t_back_cold_ms` | ms | FIELD, end |
| `t_trip_ms` | ms | SHOP, end |
| `cool_timeout` | — | only on timeout |
| `fault` | — | on abort, value is the fault text |
| `end` | — | last line of a run (bare, no `=`) |

`tau63_valid=0` means the 63 % point was already passed before the first
measuring pulse. The run then reports a bound (`tau63_bound_ms`) and not a
value: interpolating across an interval with no measured point at its
start would produce a number that looks like a measurement and is not one.

## Example

```
# gwg-tester 1.0.0
# proto=2
# lang=de
# r_series_ohm=159.1
# r_div_a_ohm=99100
# columns=t_ms;phase;U_A_mV;Vsup_mV;I_mA;R_ptc_Ohm
## GWG-Tester bereit
# gain_a=1.0047
## Auto-Null: Kanal A um 0.47 % korrigiert
# mode=FIELD
# vsup_v=18.94
# r_cold_ohm=117.8
# i_cold_ma=68.96
## Start im Modus FELD
0;HEAT;8124;18940;68.12;119.3
21;HEAT;8130;18941;68.08;119.4
...
# t_heat_ms=26224
# i_hot_ma=24.14
# r_hot_ohm=630
# i_trip_ma=46.55
## Aufheizzeit 26224 ms, Gleichgewicht bei 24.14 mA
# r63_ohm=306
## Luft-Abkuehlung, R63 = 306 Ohm
# pulse_on_ms=2.4
# pulse_period_ms=50
# duty=0.048
3;COOL;15328;19105;23.95;640.1
52;COOL;12543;18833;40.01;313.5
# tau63_ms=56
# tau63_valid=1
...
# t_back_cold_ms=1980
## wieder kalt nach 1980 ms
# end
```

## Protocol 1 (legacy)

Logs recorded before the protocol was fixed use German prose headers
(`# AUFHEIZZEIT = 26224 ms ...`) and the German phase token `AUFHEIZEN`.
The report generator reads those too; `data/2026-09-19_1412_ptc-sample.log`
is one of them. New code should only emit protocol 2.
