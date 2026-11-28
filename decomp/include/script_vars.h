// Globals of script lists, events and flags.
// Generated from the per-file extern declarations by tools/globals_driver.py; this is the one
// declaration of each of them: include this header, never redeclare one. Comments say what is known.
#ifndef GURUMIN_SCRIPT_VARS_H
#define GURUMIN_SCRIPT_VARS_H

extern "C" const char DAT_101d7ee4[];
extern "C" const char DAT_101d7f08[];
extern "C" char DAT_101dc1fc[];                           // "ch" (copied as an array, not a literal)
extern "C" int DAT_1020247c[8];                          // ring of queued script numbers (read DAT_104bfe84, write DAT_104bfe88)
extern "C" int DAT_1020d660[0x60];
extern "C" char DAT_102194f0[];                           // text of the last script error (parser callback)
extern "C" int DAT_10492d88;                              // 1 while a queued script runs
extern "C" int DAT_1049eadc;
extern "C" int DAT_1049eae4;
extern "C" int DAT_104bfe84;                              // ring read index
extern "C" int DAT_104c0208;                              // value of a PARSER_MESSAGE_STORE message (parser callback)
extern "C" char DAT_104c0338;
extern "C" char *DAT_10c5e1c4[];                           // script texts by number
extern "C" int DAT_10f20590[0x10000];
extern "C" unsigned short DAT_10f65558;                   // position in the random table

#endif
