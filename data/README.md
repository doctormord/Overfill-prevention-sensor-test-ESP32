# Measurement data

## `2026-09-19_1412_ptc-sample.log`

One FIELD-mode run on the reference probe, recorded 19 September 2026 at
14:12, 21.0 ± 0.2 °C, 60 % RH. Protocol 1 (German prose headers); the
report generator reads it.

| | |
|---|---|
| Supply | 18.94 V |
| Cold resistance / current | 117.8 Ω / 68.96 mA |
| Heat-up time | 26.2 s (limit 45 s) |
| Hot resistance / current | 630 Ω / 24.14 mA |
| τ63 in air | 56 ms |
| Back to cold | 1.98 s |
| Measuring pulse | 2.4 ms in a 50 ms raster, 4.8 % duty |
| Rows | 1160 HEAT, 25 COOL |

This is the run the example report in `docs/example-report/` was built
from. It is also the run that is worth looking at twice: the resistance
dips from 120.1 Ω to 101.9 Ω over the first 0.66 s before it starts
rising. That is not noise and not an artefact — below its Curie
temperature the probe has a *negative* temperature coefficient, so the
first thing self-heating does is lower its resistance. It is the reason
the cold value is taken with a short pulse rather than after 80 ms of
continuous current, which would read too low and drag the derived trip
threshold with it.

Each row carries its own redundancy: `I_mA` and `R_ptc_Ohm` can be
recomputed from `U_A_mV`, `Vsup_mV`, `R_SERIES` and `R_DIV_A`, so a
transcription error shows up as an inconsistency instead of as a
plausible wrong number.
