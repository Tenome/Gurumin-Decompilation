# SoundArc.dll matching decompilation

SoundArc.dll (2004-11-12) is not a DLL: its PE header says GUI executable (characteristics 0x10f, subsystem 2, base
0x400000, no relocations), and its entry point is the CRT's `WinMainCRTStartup`. It is Gurumin's sound server: a
single-instance tray program (mutex "SoundArc", `Shell_NotifyIconA`) that loads `sound3d.dll`, resolves its `I...`
entry points (`IPlay`, `IPlaySE`, `ISetListen`, ...) with `GetProcAddress`, and logs the commands it carries out to
`c:\sound.log` when that file exists. Rich header and linker: cl 13.00.9466 and link 7.00.9466 (VS .NET 2002), the
same as APP.dll and Packs.dll.

| Range | Contents |
|---|---|
| 0x401000–0x402b30 | game code, 11 functions (`triage.tsv`), the scope of this tree |
| 0x402b30–0x402c38 | import thunks (`jmp [IAT]`) |
| 0x402c38–end | static CRT |

The tools are the module-driven copies from `decomp_packs/` (see its README for the workflow); `module.json` selects
this module. The gate writes `../progress/jp/soundarc.json`.
