/*
  GWG tester -- test set for overfill prevention sensors (PTC / Kaltleiter)
  Board: LILYGO TTGO T-Display (ESP32, ST7789 135x240)

  Circuit:
      +19V --[ R_SERIES ]--+-- probe (PTC) --+-- switch --+-- GND
                           |                              |
                        node A                          (~0 V)
      node A     -> divider 100k/10k -> GPIO32
      +19V rail  -> divider 100k/10k -> GPIO33
      switch     -> GPIO25 via 220R (CPC1906Y) or gate + 10k pull-down
      Tie the 19 V supply ground to board ground. Power the board over USB-C.

  What this measures, and what it cannot
  --------------------------------------
  A Kaltleiter probe self-heats in air: resistance rises, current falls, and
  the controller reads "filling permitted". Immersed in liquid it is cooled,
  resistance falls, current rises, and filling is shut off. A service tester
  of the GPR 4 class therefore checks the heating side only, with the probe
  left in the tank -- the 1.5 s trip criterion requires the probe to be
  removed and immersed, and no instrument can produce it in situ.

  FIELD mode (probe stays in the tank)
      HEAT : time until the current is constant        -> heat-up time
      COOL : switch off, short measuring pulses        -> cooling curve in AIR
             -> tau63 and time back to the cold value
      There is no standard value for the air curve. It is a reproducible
      fingerprint, to be compared against a known good probe of the same type.

  SHOP mode (probe removed, beaker of water)
      HEAT : heat-up time
      HOLD : current stays on, immerse the probe
      TRIP : from the first current deviation until I > iTrip  -> shutoff time

  Neither threshold is guessed. The heating phase ends at thermal
  equilibrium; the shutoff threshold is formed at runtime from the cold and
  hot current of the probe actually connected.

  Serial 115200. Protocol: see docs/serial-protocol.md
      # key=value   machine readable metadata
      ## text       operator prose, localised, ignored by the report tool
      t_ms;phase;U_A_mV;Vsup_mV;I_mA;R_ptc_Ohm

  Library: TFT_eSPI (tested 2.5.43 with ESP32 core 3.3.11). The display
  configuration lives in build_opt.h IN THE SKETCH FOLDER, not in the
  library -- nothing in TFT_eSPI needs editing on any machine.

  NOT intrinsically safe. Do not test in fuel oil or in a hazardous area.
*/

#include <TFT_eSPI.h>
#include "config.h"
#include "strings.h"

// ---------------- display configuration -----------------------------------
// TFT_eSPI is a separate translation unit, so #defines from this file never
// reach it. The configuration sits in build_opt.h in the same sketch folder;
// the Arduino IDE (>= 1.8.13) passes its lines as compiler flags to ALL
// units, libraries included. The configuration therefore travels with the
// project instead of with one machine's library copy.
//
// If build_opt.h is missing or not read, TFT_eSPI silently loads its own
// default (ILI9341, 240x320). That does not fail at compile time -- it fails
// as a black display, hours later. These two guards turn it into a compile
// error instead.
#if !defined(USER_SETUP_LOADED)
  #error "build_opt.h is not being read: file missing from the sketch folder, or the IDE is too old (needs Arduino IDE 1.8.13+ / 2.x)."
#endif
#if !defined(ST7789_DRIVER) || TFT_WIDTH != 135 || TFT_HEIGHT != 240
  #error "TFT_eSPI configuration does not match the TTGO T-Display (expected ST7789, 135x240). Check build_opt.h."
#endif

#define FW_VERSION "1.0.0"
#define PROTO_VERSION 2

TFT_eSPI    tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);

enum Phase { IDLE, HEAT, HOLD, TRIP, COOL, RESULT, FAULT, DIAG };
enum Mode  { MODE_FIELD, MODE_SHOP, MODE_DIAG };

Phase phase = IDLE;
Mode  mode  = MODE_FIELD;

uint32_t tPhaseStart = 0, tImmersion = 0, tLastPulse = 0;
uint32_t tHeat = 0, tTrip = 0, tTau63 = 0, tBackCold = 0;
float    rCold = 0, rHot = 0, iHot = 0, r63 = 0, iCold = 0, iTrip = 0;
float    winSum = 0, prevMean = -1, slope = 0;
uint16_t winN = 0;
uint32_t winStart = 0, tSteadyFirst = 0;
uint8_t  steadyCnt = 0;
float    rPrev = 0;
uint32_t tPrev = 0;
bool     tau63Valid = false;   // false = R63 was already passed before pulse 1
uint32_t tFirstPulse = 0;
uint32_t pulseOnUs = 0;        // MEASURED on-time of one pulse
float    U_A = 0, VSUP = 0, I_mA = 0, R_PTC = 0;
float    mvA = 0, mvSup = 0;   // raw ADC values in mV, for diagnostics
float    gainA = 1.0f;         // auto-zero: cancels channel and divider error
float    iDisp = 0, rDisp = 0; // smoothed, display only
uint8_t  cnt = 0;
uint32_t tLastLog = 0, tLastDraw = 0;
bool     coolTimeout = false;
const char* faultMsg = "";

// ---------------- channel A linearisation ---------------------------------
float linearize(float u) { return u + CAL_SLOPE * (u - CAL_ANCHOR); }

// ---------------- measurement ---------------------------------------------
float readNodeMv(int pin) {
  uint32_t sum = 0;
  for (uint8_t i = 0; i < 12; i++) sum += analogReadMilliVolts(pin);
  return sum / 12.0f;
}

// Sample both channels INTERLEAVED: the current comes out of their
// difference, so every pair has to see the same supply ripple -- otherwise
// the ripple lands in the current reading at full amplitude. Both dividers
// must also be loaded identically; a filter capacitor on only one channel
// defeats this and was removed for exactly that reason.
//
// The pair count depends on the phase. HOLD and TRIP measure a TIME, and
// heavy averaging there delays threshold detection and corrupts the result.
// Everywhere else averaging is free. During a cooling pulse it is not free
// at all -- it is on-time, and on-time is energy into the probe.
void measureN(uint8_t pairs) {
  uint32_t sa = 0, ss = 0;
  uint16_t aMin = 65535, aMax = 0, sMin = 65535, sMax = 0;
  for (uint8_t i = 0; i < pairs; i++) {
    uint16_t va = analogReadMilliVolts(PIN_ADC_A);
    uint16_t vs = analogReadMilliVolts(PIN_ADC_SUP);
    sa += va; ss += vs;
    if (va < aMin) aMin = va;
    if (va > aMax) aMax = va;
    if (vs < sMin) sMin = vs;
    if (vs > sMax) sMax = vs;
  }
  // drop one outlier at each end
  mvA   = (sa - aMin - aMax) / (float)(pairs - 2);
  mvSup = (ss - sMin - sMax) / (float)(pairs - 2);

  VSUP = mvSup * DIV_SUP / 1000.0f;
  U_A  = linearize(mvA * DIV_A * gainA / 1000.0f);

  float u_r = VSUP - U_A;
  if (u_r < 0.01f) u_r = 0.01f;
  float i_ges = u_r / R_SERIES;     // current through the series resistor
  float i_div = U_A / R_DIV_A;      // the share of it taken by the divider
  float i_a   = i_ges - i_div;      // real current through the probe
  if (i_a < 1e-6f) i_a = 1e-6f;
  I_mA  = i_a * 1000.0f;
  R_PTC = U_A / i_a - R_SWITCH;
  if (R_PTC < 0) R_PTC = 0;

  // smoothed values for the DISPLAY only -- thresholds run on the raw ones
  if (iDisp == 0) { iDisp = I_mA; rDisp = R_PTC; }
  iDisp += 0.2f * (I_mA  - iDisp);
  rDisp += 0.2f * (R_PTC - rDisp);
}

void measure() { measureN((phase == HOLD || phase == TRIP) ? 16 : 96); }

void fetOn()  { digitalWrite(PIN_FET, GATE_INVERTED ? LOW  : HIGH); }
void fetOff() { digitalWrite(PIN_FET, GATE_INVERTED ? HIGH : LOW);  }

uint16_t coolPeriod(uint32_t t) {
  for (uint8_t i = 0; i < sizeof(COOL_PLAN) / sizeof(COOL_PLAN[0]); i++)
    if (t < COOL_PLAN[i].untilMs) return COOL_PLAN[i].periodMs;
  return 1000;
}

// Short measuring pulse. The on-time is measured, not assumed: settle delay
// plus COOL_PAIRS ADC conversions, and the conversions dominate.
void pulseMeasure() {
  uint32_t t0 = micros();
  fetOn();
  delay(PULSE_SETTLE_MS);
  measureN(COOL_PAIRS);
  fetOff();
  pulseOnUs = micros() - t0;
}

// Auto-zero with the switch off: only the divider current flows through
// R_SERIES, dropping ~27 mV across it, so node A must equal the supply.
// Whatever is left over is channel and tolerance error, not signal.
void autoZero() {
  gainA = 1.0f;
  fetOff();
  delay(30);
  float a = 0, s = 0;
  for (uint8_t i = 0; i < 8; i++) { a += readNodeMv(PIN_ADC_A); s += readNodeMv(PIN_ADC_SUP); }
  float uA = (a / 8) * DIV_A   / 1000.0f;
  float uS = (s / 8) * DIV_SUP / 1000.0f;
  if (uA < 1.0f) { Serial.println("## " S_SER_AZ_NONE); return; }
  float g = (uS - 0.027f) / uA;
  if (g > 0.9f && g < 1.1f) {
    gainA = g;
    Serial.printf("# gain_a=%.4f\n", gainA);
    Serial.printf("## " S_SER_AZ_OK "\n", (g - 1.0f) * 100);
  } else {
    Serial.printf("## " S_SER_AZ_REJECT "\n", g);
  }
}

bool edge(uint8_t pin, bool &last, uint32_t &tLast) {
  bool now = digitalRead(pin);
  bool hit = (last == HIGH && now == LOW && millis() - tLast > 200);
  if (hit) tLast = millis();
  last = now;
  return hit;
}
bool startPressed() { static bool l = HIGH; static uint32_t t = 0; return edge(BTN_START, l, t); }
bool modePressed()  { static bool l = HIGH; static uint32_t t = 0; return edge(BTN_MODE,  l, t); }

// ---------------- names ----------------------------------------------------
// Two separate functions on purpose. phaseName() is for the operator and is
// translated; phaseToken() goes into the CSV and must NOT be, or every
// report tool would need a translation table to read a log.
const char* phaseName() {
  switch (phase) {
    case IDLE:   return S_PH_IDLE;
    case HEAT:   return S_PH_HEAT;
    case HOLD:   return S_PH_HOLD;
    case TRIP:   return S_PH_TRIP;
    case COOL:   return S_PH_COOL;
    case RESULT: return S_PH_RESULT;
    case DIAG:   return S_PH_DIAG;
    default:     return S_PH_FAULT;
  }
}
const char* phaseToken() {
  switch (phase) {
    case IDLE:   return "IDLE";
    case HEAT:   return "HEAT";
    case HOLD:   return "HOLD";
    case TRIP:   return "TRIP";
    case COOL:   return "COOL";
    case RESULT: return "RESULT";
    case DIAG:   return "DIAG";
    default:     return "FAULT";
  }
}
const char* modeName() {
  switch (mode) {
    case MODE_FIELD: return S_MODE_FIELD;
    case MODE_SHOP:  return S_MODE_SHOP;
    default:         return S_MODE_DIAG;
  }
}
const char* modeToken() {
  switch (mode) {
    case MODE_FIELD: return "FIELD";
    case MODE_SHOP:  return "SHOP";
    default:         return "DIAG";
  }
}

uint16_t phaseColor() {
  switch (phase) {
    case HEAT:   return TFT_ORANGE;
    case HOLD:   return TFT_CYAN;
    case TRIP:
    case COOL:   return TFT_BLUE;
    case RESULT: return TFT_DARKGREEN;
    case DIAG:   return TFT_PURPLE;
    case FAULT:  return TFT_RED;
    default:     return TFT_DARKGREY;
  }
}

// ---------------- display --------------------------------------------------
void draw() {
  char buf[48];
  spr.fillSprite(TFT_BLACK);

  spr.fillRect(0, 0, 240, 22, phaseColor());
  spr.setTextColor(TFT_WHITE, phaseColor());
  spr.setTextDatum(TL_DATUM);
  spr.drawString(phaseName(), 5, 4, 2);
  spr.setTextDatum(TR_DATUM);
  spr.drawString(modeName(), 235, 4, 2);
  spr.setTextDatum(TL_DATUM);

  if (phase == RESULT) {
    bool okHeat = tHeat <= T_HEAT_LIMIT_MS;
    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.drawString(S_LBL_HEATING, 5, 30, 2);
    spr.setTextColor(okHeat ? TFT_GREEN : TFT_RED, TFT_BLACK);
    snprintf(buf, sizeof(buf), "%.1f s", tHeat / 1000.0);
    spr.drawString(buf, 115, 28, 4);

    if (mode == MODE_SHOP) {
      bool okTrip = tTrip <= T_TRIP_LIMIT_MS;
      spr.setTextColor(TFT_WHITE, TFT_BLACK);
      spr.drawString(S_LBL_SHUTOFF, 5, 62, 2);
      spr.setTextColor(okTrip ? TFT_GREEN : TFT_RED, TFT_BLACK);
      snprintf(buf, sizeof(buf), "%.2f s", tTrip / 1000.0);
      spr.drawString(buf, 115, 60, 4);
      spr.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
      snprintf(buf, sizeof(buf), S_LBL_R_COLD_HOT, rCold, rHot);
      spr.drawString(buf, 5, 96, 2);
      spr.setTextColor((okHeat && okTrip) ? TFT_GREEN : TFT_RED, TFT_BLACK);
      spr.drawString((okHeat && okTrip) ? S_LBL_PASS : S_LBL_FAIL, 5, 114, 2);
    } else {
      spr.setTextColor(TFT_WHITE, TFT_BLACK);
      spr.drawString(S_LBL_TAU63, 5, 62, 2);
      // unresolved: show as a bound, never as a measured value
      spr.setTextColor(tau63Valid ? TFT_CYAN : TFT_ORANGE, TFT_BLACK);
      if (tau63Valid) snprintf(buf, sizeof(buf), "%.2f s", tTau63 / 1000.0);
      else            snprintf(buf, sizeof(buf), "< %.2f s", tFirstPulse / 1000.0);
      spr.drawString(buf, 115, 60, 4);
      spr.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
      if (coolTimeout) snprintf(buf, sizeof(buf), S_LBL_NOT_COOLED, (unsigned long)(T_COOL_MAX_MS / 1000));
      else             snprintf(buf, sizeof(buf), S_LBL_COLD_AFTER, tBackCold / 1000.0);
      spr.drawString(buf, 5, 92, 2);
      snprintf(buf, sizeof(buf), S_LBL_R_COLD_HOT, rCold, rHot);
      spr.drawString(buf, 5, 112, 2);
    }
    spr.pushSprite(0, 0);
    return;
  }

  if (phase == DIAG) {
    // switch permanently on, raw values -- this is the screen you hold a
    // DMM against when calibrating
    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    snprintf(buf, sizeof(buf), "ADC A   %6.0f mV", mvA);
    spr.drawString(buf, 5, 26, 2);
    snprintf(buf, sizeof(buf), "ADC Sup %6.0f mV", mvSup);
    spr.drawString(buf, 5, 44, 2);
    spr.setTextColor(TFT_YELLOW, TFT_BLACK);
    snprintf(buf, sizeof(buf), "U_A %5.2f V   Vsup %5.2f V", U_A, VSUP);
    spr.drawString(buf, 5, 66, 2);
    spr.setTextColor(TFT_CYAN, TFT_BLACK);
    snprintf(buf, sizeof(buf), "I %6.2f mA   R %7.0f Ohm", I_mA, R_PTC);
    spr.drawString(buf, 5, 88, 2);
    spr.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    spr.drawString(S_LBL_DIAG_HINT, 5, 114, 2);
    spr.pushSprite(0, 0);
    return;
  }

  if (phase == FAULT) {
    spr.setTextColor(TFT_RED, TFT_BLACK);
    spr.drawString(faultMsg, 5, 40, 4);
    spr.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    snprintf(buf, sizeof(buf), "R = %.0f Ohm   I = %.1f mA", R_PTC, I_mA);
    spr.drawString(buf, 5, 80, 2);
    spr.drawString(S_LBL_FAULT_HINT, 5, 114, 2);
    spr.pushSprite(0, 0);
    return;
  }

  // live values: in the air phase R says more than I
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  if (phase == IDLE) {
    snprintf(buf, sizeof(buf), "%.1f", VSUP);
    spr.drawString(buf, 5, 28, 6);
    spr.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    spr.drawString("V", 150, 52, 4);
    snprintf(buf, sizeof(buf), S_LBL_NODE_A, U_A);
    spr.drawString(buf, 5, 80, 2);
  } else if (phase == COOL) {
    snprintf(buf, sizeof(buf), "%.0f", rDisp);
    spr.drawString(buf, 5, 28, 6);
    spr.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    spr.drawString("Ohm", 150, 52, 4);
    snprintf(buf, sizeof(buf), S_LBL_TARGET_R, rCold * COLD_BAND);
    spr.drawString(buf, 5, 84, 2);
  } else {
    snprintf(buf, sizeof(buf), "%.1f", iDisp);
    spr.drawString(buf, 5, 28, 6);
    spr.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    spr.drawString("mA", 150, 52, 4);
    snprintf(buf, sizeof(buf), "R %.0f Ohm", rDisp);
    spr.drawString(buf, 5, 84, 2);
    snprintf(buf, sizeof(buf), "U %.1f V", U_A);
    spr.drawString(buf, 125, 84, 2);
  }

  if (phase == IDLE) {
    spr.setTextColor(TFT_GREEN, TFT_BLACK);
    spr.drawString(S_LBL_BTN_START, 5, 98, 2);
    spr.setTextColor(TFT_YELLOW, TFT_BLACK);
    spr.drawString(S_LBL_BTN_MODE, 5, 116, 2);
  } else {
    snprintf(buf, sizeof(buf), "t = %.2f s", (millis() - tPhaseStart) / 1000.0);
    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.drawString(buf, 5, 106, 2);
    if (phase == HEAT || phase == TRIP) {
      uint32_t limit = (phase == HEAT) ? T_HEAT_LIMIT_MS : T_TRIP_LIMIT_MS;
      int w = constrain((int)(230.0 * (millis() - tPhaseStart) / limit), 0, 230);
      spr.drawRect(5, 128, 230, 6, TFT_DARKGREY);
      spr.fillRect(5, 128, w, 6, (millis() - tPhaseStart > limit) ? TFT_RED : TFT_GREEN);
    }
    if (phase == COOL && tTau63) {
      spr.setTextColor(TFT_CYAN, TFT_BLACK);
      snprintf(buf, sizeof(buf), "tau63 = %.2f s", tTau63 / 1000.0);
      spr.drawString(buf, 125, 106, 2);
    }
    if (phase == HEAT) {
      spr.setTextColor(steadyCnt ? TFT_GREEN : TFT_LIGHTGREY, TFT_BLACK);
      // no unit here: the string has to fit beside "t = nn.nn s" inside
      // 240 px, and mA/s pushed it off the panel
      snprintf(buf, sizeof(buf), "dI %+.2f  %d/%d", slope, steadyCnt, STEADY_N);
      spr.drawString(buf, 130, 106, 2);
    }
    if (phase == HOLD || phase == TRIP) {
      spr.setTextColor(TFT_YELLOW, TFT_BLACK);
      snprintf(buf, sizeof(buf), S_LBL_TARGET_I, iTrip);
      spr.drawString(buf, 125, 106, 2);
    }
  }
  spr.pushSprite(0, 0);
}

// ---------------- serial ---------------------------------------------------
void logCsv() {
  if (millis() - tLastLog < 20) return;
  tLastLog = millis();
  Serial.printf("%lu;%s;%.0f;%.0f;%.2f;%.1f\n",
                millis() - tPhaseStart, phaseToken(), U_A * 1000, VSUP * 1000, I_mA, R_PTC);
}

void goPhase(Phase p) { phase = p; tPhaseStart = millis(); cnt = 0; }

void goFault(const char* msg) {
  fetOff();
  faultMsg = msg;
  Serial.printf("# fault=%s\n", msg);
  Serial.printf("## " S_SER_FAULT "\n", msg);
  Serial.println("# end");
  goPhase(FAULT);
}

void setup() {
  Serial.begin(115200);
  fetOff();                        // set the level first, then enable output
  pinMode(PIN_FET, OUTPUT);
  fetOff();
  pinMode(BTN_START, INPUT);       // GPIO35: pull-up is on the board
  pinMode(BTN_MODE, INPUT_PULLUP);
  analogSetPinAttenuation(PIN_ADC_A,   ADC_11db);
  analogSetPinAttenuation(PIN_ADC_SUP, ADC_11db);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  spr.setColorDepth(16);
  spr.createSprite(240, 135);

  Serial.println();
  Serial.println("# gwg-tester " FW_VERSION);
  Serial.printf("# proto=%d\n", PROTO_VERSION);
  Serial.println("# lang=" GWG_LANG_TAG);
  Serial.printf("# r_series_ohm=%.1f\n", R_SERIES);
  Serial.printf("# r_div_a_ohm=%.0f\n", R_DIV_A);
  Serial.println("# columns=t_ms;phase;U_A_mV;Vsup_mV;I_mA;R_ptc_Ohm");
  Serial.println("## " S_SER_READY);
  goPhase(IDLE);
}

void loop() {
  switch (phase) {

    case IDLE:
      fetOff();
      measure();                   // idle: shows supply and node A
      if (modePressed()) mode = (Mode)((mode + 1) % 3);
      if (startPressed()) {
        autoZero();
        if (mode == MODE_DIAG) { fetOn(); goPhase(DIAG); break; }
        tTau63 = 0; tTrip = 0; tBackCold = 0; coolTimeout = false; iTrip = 0;
        tau63Valid = false; tFirstPulse = 0; pulseOnUs = 0;

        // Cold value by SHORT PULSE, not after 80 ms of continuous current.
        // At ~0.55 W the probe warms measurably within 80 ms, and because it
        // has a NEGATIVE temperature coefficient below its Curie point, the
        // reading comes out too LOW. A cold value that is too small then
        // also shifts the derived trip threshold.
        fetOn();
        delay(PULSE_SETTLE_MS);
        measureN(24);
        fetOff();
        rCold = R_PTC; iCold = I_mA;

        if (R_PTC > R_OPEN_OHM)   { goFault(S_FLT_OPEN);  break; }
        if (R_PTC < R_SHORT_OHM)  { goFault(S_FLT_SHORT); break; }
        // A cold probe MUST draw substantial current. If it does not,
        // nothing is flowing -- check the wiring or the switch.
        if (I_mA < I_MIN_COLD_MA) { goFault(S_FLT_LOW_I); break; }

        Serial.printf("# mode=%s\n", modeToken());
        Serial.printf("# vsup_v=%.2f\n", VSUP);
        Serial.printf("# r_cold_ohm=%.1f\n", rCold);
        Serial.printf("# i_cold_ma=%.2f\n", iCold);
        Serial.printf("## " S_SER_START "\n", modeName());

        fetOn();                   // now the heating phase proper
        winSum = 0; winN = 0; prevMean = -1; steadyCnt = 0; slope = 0;
        winStart = millis();
        goPhase(HEAT);
      }
      break;

    case HEAT: {
      measure();
      logCsv();
      winSum += I_mA; winN++;
      if (millis() - winStart >= STEADY_WIN_MS && winN > 0) {
        float mean = winSum / winN;
        winSum = 0; winN = 0; winStart = millis();
        if (prevMean >= 0) {
          slope = mean - prevMean;
          if (fabs(slope) < STEADY_SLOPE_MA) {
            // Timestamp the FIRST quiet window. The windows that follow only
            // confirm it; they are not part of the heat-up time.
            if (steadyCnt == 0) tSteadyFirst = millis() - tPhaseStart;
            steadyCnt++;
          } else steadyCnt = 0;
        }
        prevMean = mean;

        if (steadyCnt >= STEADY_N) {
          if (mean > iCold * HEAT_DROP_MIN) {   // constant, but never heated
            goFault(S_FLT_NO_HEAT); break;
          }
          tHeat = tSteadyFirst;
          rHot  = R_PTC;
          iHot  = mean;                 // the averaged window, not one sample
          r63   = rHot - 0.632f * (rHot - rCold);
          iTrip = iHot + TRIP_FRAC * (iCold - iHot);

          Serial.printf("# t_heat_ms=%lu\n", tHeat);
          Serial.printf("# i_hot_ma=%.2f\n", iHot);
          Serial.printf("# r_hot_ohm=%.0f\n", rHot);
          Serial.printf("# i_trip_ma=%.2f\n", iTrip);
          Serial.printf("## " S_SER_HEAT_DONE "\n", tHeat, iHot);
          Serial.printf("## " S_SER_TRIP_THR "\n", iTrip, iCold, iHot);

          if (mode == MODE_SHOP) goPhase(HOLD);
          else {
            fetOff();
            rPrev = rHot; tPrev = 0;
            tau63Valid = true; tFirstPulse = 0;
            // First pulse IMMEDIATELY, not after one period -- that off-by-
            // one-period is exactly what put the first cooling sample at
            // 529 ms and produced a tau63 of 382 ms that was an artefact.
            tLastPulse = millis() - coolPeriod(0);
            Serial.printf("# r63_ohm=%.0f\n", r63);
            Serial.printf("## " S_SER_COOL "\n", r63);
            goPhase(COOL);
          }
          break;
        }
      }
      if (millis() - tPhaseStart > T_HEAT_MAX_MS) goFault(S_FLT_NO_STEADY);
      break;
    }

    case HOLD:
      measure();
      logCsv();
      // If the current keeps falling, equilibrium was not quite reached --
      // track the hot value AND the threshold derived from it.
      if (I_mA < iHot) {
        iHot  = I_mA;
        iTrip = iHot + TRIP_FRAC * (iCold - iHot);
      }
      if (I_mA > iHot + I_DEVIATION_MA) {
        if (++cnt >= 2) {
          tImmersion = millis();
          Serial.println("## " S_SER_IMMERSION);
          goPhase(TRIP);
          tPhaseStart = tImmersion;
        }
      } else cnt = 0;
      break;

    case TRIP:
      measure();
      logCsv();
      if (I_mA > iTrip) {
        if (++cnt >= DEBOUNCE_N) {
          tTrip = millis() - tImmersion;
          fetOff();
          Serial.printf("# t_trip_ms=%lu\n", tTrip);
          Serial.println("# end");
          goPhase(RESULT);
        }
      } else cnt = 0;
      if (millis() - tPhaseStart > T_TRIP_MAX_MS) goFault(S_FLT_NO_TRIP);
      break;

    case COOL: {
      if (millis() - tLastPulse < coolPeriod(millis() - tPhaseStart)) break;
      tLastPulse = millis();
      pulseMeasure();
      uint32_t t = millis() - tPhaseStart;

      if (!tFirstPulse) {
        tFirstPulse = t;
        // Report the duty cycle from the MEASURED on-time, in the tightest
        // raster, which is where it is worst.
        float onMs = pulseOnUs / 1000.0f;
        float duty = onMs / COOL_PLAN[0].periodMs;
        Serial.printf("# pulse_on_ms=%.1f\n", onMs);
        Serial.printf("# pulse_period_ms=%u\n", COOL_PLAN[0].periodMs);
        Serial.printf("# duty=%.3f\n", duty);
        Serial.printf("## " S_SER_DUTY "\n", onMs, COOL_PLAN[0].periodMs, duty * 100);
        if (duty > DUTY_WARN) {
          Serial.println("# duty_warn=1");
          Serial.println("## " S_SER_DUTY_WARN);
        }
      }

      Serial.printf("%lu;COOL;%.0f;%.0f;%.2f;%.1f\n",
                    t, U_A * 1000, VSUP * 1000, I_mA, R_PTC);

      // tau63 by linear interpolation between two pulses. If that happens
      // inside the FIRST interval, the interpolation rests on no measured
      // point at all -> flag it as unresolved instead of printing a number
      // that looks like a measurement.
      if (!tTau63 && rPrev > r63 && R_PTC <= r63) {
        bool firstInterval = (tPrev == 0);
        float f = (rPrev - r63) / (rPrev - R_PTC);
        tTau63 = tPrev + (uint32_t)(f * (t - tPrev));
        if (firstInterval) {
          tau63Valid = false;
          Serial.printf("# tau63_bound_ms=%lu\n# tau63_valid=0\n", t);
          Serial.printf("## " S_SER_TAU_UNRES "\n", t);
        } else {
          Serial.printf("# tau63_ms=%lu\n# tau63_valid=1\n", tTau63);
        }
      }
      rPrev = R_PTC; tPrev = t;

      if (R_PTC < rCold * COLD_BAND) {
        if (++cnt >= 2) {
          tBackCold = t;
          if (!tTau63) { tTau63 = t; tau63Valid = false; }
          Serial.printf("# t_back_cold_ms=%lu\n", tBackCold);
          Serial.printf("## " S_SER_COLD "\n", tBackCold);
          Serial.println("# end");
          goPhase(RESULT);
        }
      } else cnt = 0;

      if (t > T_COOL_MAX_MS) {
        coolTimeout = true;
        if (!tTau63) { tTau63 = t; tau63Valid = false; }
        Serial.println("# cool_timeout=1");
        Serial.println("## " S_SER_TIMEOUT);
        Serial.println("# end");
        goPhase(RESULT);
      }
      break;
    }

    case DIAG:
      measure();
      if (millis() - tLastLog > 250) {
        tLastLog = millis();
        Serial.printf("# diag mv_a=%.0f mv_sup=%.0f u_a=%.2f vsup=%.2f i_ma=%.2f r_ohm=%.0f\n",
                      mvA, mvSup, U_A, VSUP, I_mA, R_PTC);
      }
      if (startPressed() || millis() - tPhaseStart > 300000) { fetOff(); goPhase(IDLE); }
      break;

    case RESULT:
    case FAULT:
      fetOff();
      if (startPressed()) goPhase(IDLE);
      break;
  }

  if (millis() - tLastDraw > 120) { tLastDraw = millis(); draw(); }
}
