# Bill of materials

One unit. Reference designators match `gwg-tester.kicad_sch` /
`gwg-tester.svg`.

| Ref | Value | Type | Notes |
|---|---|---|---|
| MOD1 | LILYGO TTGO T-Display | ESP32 + ST7789 135×240 | powered from USB-C |
| U1 | CPC1906Y | OptoMOS solid state relay, 600 V / 100 mA | see "switching element" |
| R1 | 159.1 Ω | ≥ 1 W total | **measure and enter in `config.h`** |
| R2 | 1 MΩ | 1 % metal film, 1/4 W | across the probe terminals |
| R3 | 220 Ω | 1/4 W | LED current for U1, ≈ 9 mA at 3.3 V |
| R4, R6 | 100 kΩ | 1 % metal film, 1/4 W | divider top |
| R5, R7 | 10 kΩ | 1 % metal film, 1/4 W | divider bottom |
| C1, C2 | 10 nF | **do not populate** | see note 2 on the schematic |
| J1 | 2-pin terminal | — | 19 V DC in |
| J2 | 2-pin terminal | — | probe, polarity does not matter |
| — | 19 V DC supply | ≥ 100 mA, adjustable preferred | a laptop brick works |

## R1, the series resistor

0.55 W is dissipated in R1 with a cold probe, so it is built from several
resistors in parallel rather than from one part:

| Build | Value | Per part | Parts needed |
|---|---|---|---|
| 6 × 1 kΩ parallel | 166.7 Ω | 0.10 W | 1/4 W is comfortable |
| 3 × 470 Ω parallel | 156.7 Ω | 0.21 W | 1/4 W is at its limit |
| 150 Ω + 10 Ω series | 160.0 Ω | 0.59 W in the 150 Ω | needs a 1 W part |

The exact value does not matter; knowing it does. Measure the finished
network with the probe terminals open and one end lifted, and enter the
result as `R_SERIES` in `firmware/gwg_tester/config.h`.

Do not derive `R_SERIES` from the instrument's own two ADC channels. Both
channels are calibrated against the same voltage, so any value obtained
that way is circular and will agree with itself no matter how wrong it is.
Measure the current through the branch with a multimeter in series and
divide: that is an independent measurement, and it is what the 159.1 Ω in
this repository came from.

## Switching element

The low side is switched, so the switch sits between the probe and ground.
Three things work; two of them have traps.

**CPC1906Y (used here).** A solid state relay. GPIO25 drives the input LED
through R3; the output is isolated and polarity independent, so it cannot
be wired the wrong way round. Slow by relay standards, irrelevant at the
time scales measured here.

**A logic-level MOSFET.** Works, if it really is logic level. `RDS(on)` has
to be specified at `VGS = 2.5 V` or lower, not at 10 V. Add a 10 kΩ pull-down
from gate to ground so the probe cannot be energised while the ESP32 boots.

**A standard MOSFET with a 3.3 V gate.** Does not work. It will conduct
partially, the drain sits at several volts, and the measured probe
resistance comes out high and unstable in a way that looks exactly like a
bad solder joint. Modules sold as "MOSFET switch" are frequently built
around parts whose datasheet specifies `RDS(on)` at 10 V only, whatever the
product description says. If a known 470 Ω resistor reads as tens of
kilohms and bridging the switch fixes it, this is why — measure `VDS` with
the switch commanded on before looking anywhere else.

Set `GATE_INVERTED` in `config.h` to `true` only if an inverting NPN level
shifter sits between GPIO25 and the gate.

## Wiring notes

- Tie the 19 V supply ground to module ground. Without that return path,
  node A floats and every reading is meaningless.
- GPIO32 and GPIO33 are ADC1. ADC2 is unusable while WiFi is active, which
  is why no other pins will do.
- GPIO0 and GPIO35 are the on-board buttons and need no external wiring.
  GPIO0 is a strapping pin: do not pull it low at reset.
- Keep both dividers identical, including their loading. The current is the
  difference of the two channels, so anything that affects one channel and
  not the other goes straight into the current reading.
