#pragma once
/*
  Compile-time language selection. GWG_LANG comes from config.h, or from
  build_opt.h with -DGWG_LANG=1 .

  Both string sets are plain #defines, so exactly one of them ends up in the
  binary -- no table, no lookup, no flash cost for the language not built.
  Adding a language means adding a file and one #elif; the sketch body does
  not change, because it never contains a literal the operator can read.
*/

#if GWG_LANG == 0
  #include "strings_de.h"
  #define GWG_LANG_TAG "de"
#elif GWG_LANG == 1
  #include "strings_en.h"
  #define GWG_LANG_TAG "en"
#else
  #error "GWG_LANG must be 0 (German) or 1 (English)."
#endif

/*
  Guard against a half-finished translation. A missing #define would
  otherwise compile as an undefined identifier only where it is used, and a
  string used on one screen in one mode can hide for a long time.
*/
#if !defined(S_PH_IDLE)   || !defined(S_PH_FAULT)  || !defined(S_MODE_DIAG) || \
    !defined(S_LBL_PASS)  || !defined(S_LBL_FAIL)  || !defined(S_FLT_OPEN)  || \
    !defined(S_SER_READY) || !defined(S_SER_FAULT) || !defined(S_LBL_TAU63)
  #error "Incomplete string set: compare your strings_*.h against strings_de.h."
#endif
