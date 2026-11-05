#pragma once
/*
  German operator strings.

  Deliberately ASCII only ("ABKUEHLEN", not "ABKÜHLEN"): the TFT_eSPI bitmap
  fonts 2/4/6 carry no reliable code page above 0x7F, so umlauts would come
  out as garbage glyphs on the display. Transliterating here is visible and
  predictable; relying on a code page is neither.
*/

#define S_PH_IDLE        "BEREIT"
#define S_PH_HEAT        "AUFHEIZEN"
#define S_PH_HOLD        "JETZT EINTAUCHEN"
#define S_PH_TRIP        "ABKUEHLEN"
#define S_PH_COOL        "ABKUEHLEN (LUFT)"
#define S_PH_RESULT      "ERGEBNIS"
#define S_PH_DIAG        "DIAGNOSE"
#define S_PH_FAULT       "FEHLER"

#define S_MODE_FIELD     "FELD"
#define S_MODE_SHOP      "WERKSTATT"
#define S_MODE_DIAG      "DIAG"

#define S_LBL_HEATING    "Aufheizen"
#define S_LBL_SHUTOFF    "Abschalten"
#define S_LBL_TAU63      "tau63 Luft"
#define S_LBL_NODE_A     "Knoten A %.1f V"
#define S_LBL_R_COLD_HOT "R kalt %.0f  heiss %.0f Ohm"
#define S_LBL_COLD_AFTER "kalt nach %.0f s"
#define S_LBL_NOT_COOLED "nicht ausgekuehlt (>%lu s)"
#define S_LBL_TARGET_R   "Ziel < %.0f Ohm"
#define S_LBL_TARGET_I   "Ziel > %.1f mA"
#define S_LBL_PASS       "BESTANDEN"
#define S_LBL_FAIL       "NICHT BESTANDEN"
#define S_LBL_BTN_START  "rechts = Start"
#define S_LBL_BTN_MODE   "links = Modus wechseln"
#define S_LBL_DIAG_HINT  "FET EIN - Taste = Ende"
#define S_LBL_FAULT_HINT "Taste = neuer Test"

#define S_FLT_OPEN       "kein GWG / Bruch"
#define S_FLT_SHORT      "Kurzschluss"
#define S_FLT_LOW_I      "Strom zu klein"
#define S_FLT_NO_HEAT    "heizt nicht auf"
#define S_FLT_NO_STEADY  "kein Gleichgewicht"
#define S_FLT_NO_TRIP    "schaltet nicht ab"

// Serial prose. Printed behind "## " and ignored by the report generator.
#define S_SER_READY      "GWG-Tester bereit"
#define S_SER_AZ_OK      "Auto-Null: Kanal A um %.2f %% korrigiert"
#define S_SER_AZ_NONE    "Auto-Null uebersprungen: keine Versorgungsspannung"
#define S_SER_AZ_REJECT  "Auto-Null VERWORFEN (Faktor %.3f) - Spannungsteiler pruefen"
#define S_SER_START      "Start im Modus %s"
#define S_SER_HEAT_DONE  "Aufheizzeit %lu ms, Gleichgewicht bei %.2f mA"
#define S_SER_TRIP_THR   "Abschaltschwelle %.2f mA (kalt %.2f / heiss %.2f)"
#define S_SER_COOL       "Luft-Abkuehlung, R63 = %.0f Ohm"
#define S_SER_DUTY       "Messpuls %.1f ms ein im %u-ms-Raster -> Einschaltdauer %.0f %%"
#define S_SER_DUTY_WARN  "WARNUNG: Einschaltdauer zu hoch, die Messpulse heizen nach"
#define S_SER_TAU_UNRES  "tau63 nicht aufgeloest: R63 lag vor dem ersten Puls (%lu ms)"
#define S_SER_IMMERSION  "Eintauchen erkannt"
#define S_SER_COLD       "wieder kalt nach %lu ms"
#define S_SER_TIMEOUT    "Zeitueberschreitung in der Abkuehlphase"
#define S_SER_FAULT      "Abbruch: %s"
