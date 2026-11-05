#pragma once
/*
  config.h -- everything that depends on YOUR hardware sample.
  Change values here, not in the sketch body.
*/

// ---------------------------------------------------------------- language --
// 0 = German, 1 = English. Only the operator-facing strings change:
// the display, and the "## " prose lines on the serial port. The CSV data
// rows, the phase tokens and the "# key=value" metadata lines are NOT
// translated -- they are the machine interface and must stay stable so that
// one report generator reads logs from any unit.
//
// Can also be overridden from build_opt.h with  -DGWG_LANG=1 .
#ifndef GWG_LANG
  #define GWG_LANG 0
#endif

// -------------------------------------------------------------------- pins --
#define PIN_FET      25      // switch control
#define PIN_ADC_A    32      // ADC1_4  node A
#define PIN_ADC_SUP  33      // ADC1_5  supply rail
#define BTN_START    35      // right button, active LOW, board pull-up
#define BTN_MODE      0      // left button, active LOW (strapping pin)

// Switching element: CPC1906Y OptoMOS solid state relay.
//   GPIO25 --[220R]--> pin1 (LED anode), pin2 (cathode) --> GND,  ~9 mA
//   pin3/pin4 = output, galvanically isolated, polarity independent.
// Normal logic (HIGH = on). Set true only when an inverting NPN level
// shifter drives a MOSFET gate.
#define GATE_INVERTED false

// --------------------------------------------------------------- resistors --
// Closed-state resistance of the switching element, subtracted from the
// result. Determine in DIAG against a known resistor: shown value minus
// DMM value. Measured 1 mV across the device at 38.6 mA -> below resolution.
const float R_SWITCH = 0.0f;

// Series resistor. NOT taken from the colour code and NOT back-calculated
// from the two ADC channels -- both of those are circular here. Measured
// directly: 38.6 mA through the branch at 6.14 V across the resistor.
const float R_SERIES = 159.1f;

// Divider ratios, calibrated against a UT71C at 18.821 V (raw reading was
// 19.04 / 19.07 V). This absorbs resistor tolerance AND ADC gain error.
const float DIV_A   = 10.874f;
const float DIV_SUP = 10.856f;

// Parasitic path from node A to GND, divided out in measure():
// the 110k measuring divider in parallel with the 1M across the probe
// terminals -> 99.1k. The 1M also guarantees a defined current with no
// probe connected, so open-circuit detection cannot sit at an undefined
// node and read something plausible.
const float R_DIV_A = 99100.0f;

// ------------------------------------------------- channel A linearisation --
// From 6 comparisons against a UT71C (100R..990R test resistors): the
// deviation runs linearly with voltage, so it is not curvature but ADC
// offset (~26 mV at the pin) plus ~1.5 % gain error. A stepwise table would
// freeze the read-off noise of those 6 points into the firmware; a straight
// line does not. The anchor stays at 18.821 V, where DIV_A was calibrated.
// Residual +/-0.04 V, limited by the read-off resolution of the reference.
//
// Recalibrating: connect a known resistor, compute the true node voltage
// from U = Vsup * Rp/(R_SERIES + Rp) with Rp = R_DMM || R_DIV_A, then
//   CAL_SLOPE = (U_true - U_shown) / (U_shown - CAL_ANCHOR)
const float CAL_ANCHOR = 18.821f;
const float CAL_SLOPE  = 0.0155f;

// ---------------------------------------------------------- test criteria --
// The heating phase does not end at a fixed current but when the probe has
// reached thermal equilibrium -- the same thing a GPR 4 means by "operating
// temperature reached". The result therefore no longer hangs on a guessed
// switch-over point.
const uint32_t STEADY_WIN_MS   = 1000;   // averaging window for the slope
const float    STEADY_SLOPE_MA = 0.15f;  // below this the current is constant
const uint8_t  STEADY_N        = 3;      // this many consecutive windows
const float    HEAT_DROP_MIN   = 0.5f;   // current MUST fall below 50 % of cold

// The shutoff threshold is derived at runtime from the two measured corner
// points of the probe actually connected:
//     iTrip = iHot + TRIP_FRAC * (iCold - iHot)
// iCold is fixed at start, iHot at the end of the heating phase. 0.5 = half
// way back to the cold value. Larger -> detected later, longer trip time.
const float TRIP_FRAC      = 0.5f;
const float I_MIN_COLD_MA  = 20.0f;      // sanity at start (125R -> ~67 mA)
const float I_DEVIATION_MA = 0.5f;       // trigger for "immersion detected"

const uint32_t T_HEAT_MAX_MS   = 60000;  // abort limits
const uint32_t T_TRIP_MAX_MS   = 30000;
const uint32_t T_COOL_MAX_MS   = 180000;
const uint32_t T_HEAT_LIMIT_MS = 45000;  // pass criterion, heating
const uint32_t T_TRIP_LIMIT_MS = 1500;   // pass criterion, shutoff
const uint8_t  DEBOUNCE_N      = 3;

// ----------------------------------------- pulsed measurement while cooling --
// In the air cooling phase the probe must not be heated by its own
// measurement. Each sample is a short pulse; what matters is the pulse
// ENERGY, i.e. the real on-time, not the nominal settle delay. The on-time
// is therefore measured with micros() and reported -- an earlier version
// computed the duty cycle from PULSE_SETTLE_MS alone, ignored the averaging
// time, and reported 24 % where the true figure was several times that.
// The cooling curve measured that way is biased: the probe reheats.
const uint16_t PULSE_SETTLE_MS = 2;   // settle time before sampling
const uint8_t  COOL_PAIRS      = 8;   // sample pairs per pulse (keeps it short)
const float    DUTY_WARN       = 0.08f;

// Cooling is fast at first and sluggish later -> staggered raster.
struct CoolStep { uint32_t untilMs; uint16_t periodMs; };
const CoolStep COOL_PLAN[] = {
  {          1000,   50 },   // first second: tau63 lives here
  {          3000,  200 },
  {         10000,  500 },
  { 0xFFFFFFFFUL, 1000 },
};

const float COLD_BAND = 1.05f;        // "cold again" = R < 1.05 * R_cold

// --------------------------------------------------------- fault detection --
const float R_OPEN_OHM  = 20000.0f;
const float R_SHORT_OHM = 5.0f;
