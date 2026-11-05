#pragma once
/*
  English operator strings.

  Kept to the same lengths as the German set where it matters: the display is
  240 px wide and the header line has to hold phase name and mode side by
  side, so "IMMERSE NOW" rather than "IMMERSE THE PROBE NOW".
*/

#define S_PH_IDLE        "READY"
#define S_PH_HEAT        "HEATING"
#define S_PH_HOLD        "IMMERSE NOW"
#define S_PH_TRIP        "COOLING"
#define S_PH_COOL        "COOLING (AIR)"
#define S_PH_RESULT      "RESULT"
#define S_PH_DIAG        "DIAGNOSTICS"
#define S_PH_FAULT       "FAULT"

#define S_MODE_FIELD     "FIELD"
#define S_MODE_SHOP      "SHOP"
#define S_MODE_DIAG      "DIAG"

#define S_LBL_HEATING    "Heat-up"
#define S_LBL_SHUTOFF    "Shutoff"
#define S_LBL_TAU63      "tau63 air"
#define S_LBL_NODE_A     "node A %.1f V"
#define S_LBL_R_COLD_HOT "R cold %.0f  hot %.0f Ohm"
#define S_LBL_COLD_AFTER "cold after %.0f s"
#define S_LBL_NOT_COOLED "not cooled down (>%lu s)"
#define S_LBL_TARGET_R   "target < %.0f Ohm"
#define S_LBL_TARGET_I   "target > %.1f mA"
#define S_LBL_PASS       "PASS"
#define S_LBL_FAIL       "FAIL"
#define S_LBL_BTN_START  "right = start"
#define S_LBL_BTN_MODE   "left = change mode"
#define S_LBL_DIAG_HINT  "switch ON - button = exit"
#define S_LBL_FAULT_HINT "button = new test"

#define S_FLT_OPEN       "no probe / open"
#define S_FLT_SHORT      "short circuit"
#define S_FLT_LOW_I      "current too low"
#define S_FLT_NO_HEAT    "does not heat up"
#define S_FLT_NO_STEADY  "no equilibrium"
#define S_FLT_NO_TRIP    "does not shut off"

// Serial prose. Printed behind "## " and ignored by the report generator.
#define S_SER_READY      "GWG tester ready"
#define S_SER_AZ_OK      "auto-zero: channel A corrected by %.2f %%"
#define S_SER_AZ_NONE    "auto-zero skipped: no supply voltage"
#define S_SER_AZ_REJECT  "auto-zero REJECTED (factor %.3f) - check the dividers"
#define S_SER_START      "start in %s mode"
#define S_SER_HEAT_DONE  "heat-up time %lu ms, equilibrium at %.2f mA"
#define S_SER_TRIP_THR   "shutoff threshold %.2f mA (cold %.2f / hot %.2f)"
#define S_SER_COOL       "air cooling, R63 = %.0f Ohm"
#define S_SER_DUTY       "measuring pulse %.1f ms on in a %u ms raster -> duty %.0f %%"
#define S_SER_DUTY_WARN  "WARNING: duty too high, the measuring pulses reheat the probe"
#define S_SER_TAU_UNRES  "tau63 unresolved: R63 was passed before the first pulse (%lu ms)"
#define S_SER_IMMERSION  "immersion detected"
#define S_SER_COLD       "cold again after %lu ms"
#define S_SER_TIMEOUT    "timeout in the cooling phase"
#define S_SER_FAULT      "aborted: %s"
