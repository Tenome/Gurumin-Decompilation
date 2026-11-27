# Packs.dll matching decompilation

Packs.dll (2004-06-23) is Gurumin's image and archive helper: BMP packing (`BMPPack`/`BMPUnpack`, `MMV3`), BMP and
JPEG files (`BMPLoad`, `BMPSave`, `SaveJpeg`) and a general compressor (`ExecPack`/`ExecUnpack`). Rich header and
linker: cl 13.00.9466 and link 7.00.9466, the same VS .NET 2002 build as APP.dll, so `env.bat` uses the same compiler
and `build.bat` the same flags (`/O2 /Ob1 /Op /Gy /GS /GX`).

Layout of `.text`:

| Range | Contents |
|---|---|
| 0x10001000–0x10007170 | game code, 51 functions (`triage.tsv`), the scope of this tree |
| 0x10007170–0x1000fa6c and 0x1002e0e6–end | static CRT (unaligned, not compiled with `/Gy`) |
| 0x1000fa6c–0x1002e0e6 | IJG libjpeg 6b ("Copyright (C) 1998, Thomas G. Lane"), a static library built without `/Gy` |

Ghidra's export misses three exports (`BMPUnpack`, `SaveJpeg`, `MMV3`: only in `extra.c`); `module.json` lists them
and `tools/gimg.py` adds them. Ghidra's sizes also leave out epilogues and switch tables, so `gimg.inventory()` sizes
each game function to the next function start, minus its `nop` padding.

## Workflow

From `decomp_packs/`:

1. `python tools/triage.py` once (writes `triage.tsv`; it is derived from the game binary and stays local).
2. `cmd /c build.bat <file.cpp>` builds one file and prints `MATCH`/`DIFF` per function.
3. Module check: `python tools/build_all.py` (writes `build/report.json`), then `python tools/check_matches.py`
   (exits 1 if a match was lost against the committed `progress/jp/report.json`). The real gate is APP.dll's:
   `python decomp/tools/build_all.py --report progress/jp/report.json` builds this tree too and merges its units.

`tools/asm.py <addr>` disassembles, `tools/fn.py <addr>` prints Ghidra's decompilation. Functions whose names carry
no address (methods, exports) are mapped in `obj_symbols.tsv`. The hard rules of `../AGENTS.md` apply unchanged;
types go in `include/`.
