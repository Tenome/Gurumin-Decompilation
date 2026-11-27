# sound3d.dll matching decompilation

sound3d.dll (2004-12-27) is Gurumin's sound engine. SoundArc.dll (the sound server, `decomp_soundarc/`) loads it and
calls its exported `I...` API (`IInit`, `IPlay`, `IPlaySE`, `ISetListen`, `IVolFade`, ...) plus `MixOpen`/`MixClose`
and `GetParam`/`SetParam`. It plays through DirectSound 8 (`DirectSoundCreate8`) and the ACM stream API. Rich header:
the game objects are cl 13.00.9466 (VS .NET 2002), built like APP.dll (`/O2 /Ob1 /Op /Gy /GS /GX`, confirmed by a
probe of `ISetSEVol` and `IReset`).

| Range | Contents |
|---|---|
| 0x10001000–0x1000a800 | game code, 116 functions, 38 KB (`triage.tsv`), the scope of this tree |
| 0x1000a800–0x10034670 | import thunks, static CRT and MFC (unaligned) |
| 0x10034670–0x100482d0 | libVorbis ("Xiphophorus libVorbis I 20011217") and libogg, built with `/Gy` |
| 0x100482d0–end | more CRT/MFC |

The tools are the module-driven copies from `decomp_packs/` (see its README); `module.json` selects this module.
The gate writes `../progress/jp/sound3d.json`.
