# Gurumin (JP, 2006) matching decompilation — notes

Do not publish game files, extracted assets or decompiled code beyond what the project's license allows.

## Targets
Source: `Gurumin\Gurumin JP\` (retail JP install). Pristine copies + SHA-256 in `re\bin\` (never modify).

| Binary | Size | Linker/timestamp | Role |
|---|---|---|---|
| `game.exe` | 692,224 | 7.0 / 2006-05-08 | small launcher |
| `bin\APP.dll` | 2,375,680 | 7.0 / 2006-05-09 | **main game code (first target)**; D3D9 build |
| `bin\APP8.dll` | 2,359,296 | 7.0 / 2006-05-09 | same code, D3D8 build (`.data1` section extra) |
| `Packs/sound3d/Update2/SoundArc/PADDLL.dll` | small | 2004 | helper DLLs, later |

The English Steam `game.exe` (6 MB, linker 12.0 / VS2013 Update 4, 2015) is a rebuild with its own matching
project in `decomp_en/` (cl 18.00.31101, flags `/O2 /Oy- /GS- /EHsc`; read `decomp_en/NOTES.md` and
`decomp_en/HANDOFF.md`). JP compiler habits below do not apply to it.

## Compiler (decoded Rich header)
APP.dll / APP8.dll: bulk of objects built with product build **9466** (Visual Studio .NET 2002, cl 13.00.9466, linker 7.00):
54 C++ (prod 0x40) + 141 C (0x1c) + 32 (0x1d, MASM/resource-ish) objects. A few objects (0x5d/0x5f/0x60) carry build 4035
(13.10.4035, a 2003-era pre-release/DDK), and some imported libs use 9162/9178/9210 (SDK/CRT libs).
Available here: VS .NET 2003 SP1 (cl 13.10.6030) and C++ Toolkit 2003 (13.10.3052). VC6 (12.00) is **not** the
build compiler. The user chose: try 2003 first; a 2002 (13.00.9466) toolchain would be the exact build — ask if findable.
Expect codegen differences between 13.00 and 13.10 (scheduling/inlining); judge each function empirically.

## Layout
- `re\` Ghidra project (`run_ghidra_export.bat` → `re\export\`), scripts copied from the TOD/TSH projects.
- `decomp\` source (`src\`), headers (`include\`), `env.bat` (VS2003 environment), build output (`build\`).

## Status
- 2026-10-29: skeleton created. Ghidra analysis of APP.dll done (`re\export\APP.dll\`: `functions.tsv`, `decompiled.c`,
  `data_refs.tsv`, `imports.tsv`, `exports.tsv`). Image base 0x10000000; one export `GuruMain` (0x1013dfb0); entry 0x101b5a8b.
  3,085 functions, ~1.85 MB of code. Imports: kernel32, user32, imm32, gdi32, winmm, shell32, ole32, advapi32,
  **d3d9 (Direct3DCreate9) and dinput8** only — rendering/input go through COM, so hook points for modernisation are few.
  Largest functions: 0x1011d810 (107 KB), 0x100d24a0 (55 KB), 0x10151ba0 (43 KB), 0x10104640 (33 KB), 0x100edea0 (32 KB).
  Next: locate the CRT boundary (entry 0x101b5a8b is late, CRT likely follows ~0x1018xxxx+), test-compile a few small
  functions with VS2003 to gauge 13.00-vs-13.10 divergence, then port the TSH tooling (gen/build/compare_obj).
- 2026-10-29: toolchain proven. `decomp\build.bat <file.cpp>` compiles with VS2003 (`/O2 /Gy`, C++) and byte-compares every
  `FUN_xxxxxxxx` / `M_xxxxxxxx` function with APP.dll (`tools\compare_obj.py`; relocations masked; the compared length includes
  the inter-function alignment padding, which also matches). Helpers: `tools\asm.py <addr>` (original), `tools\objasm.py <obj> <name>`
  (ours), `tools\fn.py <addr>` (Ghidra C). Run via `& .\build.bat` in PowerShell (cmd /c and bash fail to find it). Python:
  `C:\Users\Michael\AppData\Local\Programs\Python\Python313\python.exe` (has capstone).
  First matches in `src\app_0001.cpp`: 0x10001000, 0x10001010 (thiscall method; two separate `if (x == 0) return 0;` — the `&&`/`||`
  forms give a different block layout), 0x10001050. 3/3 trivial functions match with VS2003 `/O2` → 13.10 is viable at least for
  simple code; larger functions will show whether 13.00-vs-13.10 scheduling differs.
  CRT/statics start around 0x101b29e9 (`__security_check_cookie`, stdio, etc.); game code is 0x10001000–~0x101b0000;
  0x101d0000+ is ~1,200 tiny import thunks. `/GS` cookie present (VS2002+).
- Sample so far (6 small functions, `src\app_0001.cpp`, `src\app_0002.cpp`): 6/6 match with VS2003 `/O2 /Gy` (incl. calls, imports, globals).
  Needs `#include <windows.h>` (VS2003 PlatformSDK path in `env.bat` works). Callees' calling conventions come from the original
  (`ret N` = `__stdcall`; no stack cleanup after a call = callee cleans).
  **First divergence: 0x1000da40** (float: `(float)int` stored through an `fstp [esp+8]` / integer reload spill, stores
  of the same float twice). VS2003 `/O2`, `/Op`, `/fp:*` all give different code (`fild`/`fst` kept on the FPU stack). Likely
  13.00-vs-13.10 float codegen; unresolved. Float-heavy functions (math, camera, animation) may be unmatchable without cl 13.00.9466.
  Candidate next steps: (1) look for a VS .NET 2002 compiler (the exact build); (2) match the integer/string/COM-glue functions first
  with VS2003; (3) build a function-triage table (size, float ops, callees) to prioritise.
- Triage: `decomp\tools\triage.py` → `decomp\triage.tsv` (game code 0x10001000–0x101b0000 = 1,212 functions; 513 contain x87 ops;
  356 are ≤120 bytes without FP). Likely a single huge "Game" object (members at +0x3ee694 etc.), thiscall methods named `M_xxxxxxxx`.
- `src\app_0003.cpp`: 0x100022b0, 0x10002300, 0x10008430 match. Lessons: (1) trust the asm, not Ghidra's conditions
  (`-x < 3999 || x == -3999` was really `x = -x; if (x > 3999)`); (2) compare operand order matters (`x > right`, not `right < x`);
  (3) a shared `return 1` block can reveal the original's branch structure. Running tally: 9 of 10 attempted functions match
  (the one failure is the float function 0x1000da40).
- 2026-11-16: 19 functions match (`src\app_0001..0005.cpp`, `build_all.bat` totals them). Shared layout in `include\game.h`
  (hand-written `struct Game`, thiscall methods `M_xxxxxxxx`; slot arrays at +0x8c/+0xa4/+0x110, flag array +0x3ee694).
  Lessons: pointer-to-member needs a *complete* class or cl emits 16-byte MI pointers (`@28` instead of `@16`); `for (i = n-1; i >= 0; i--)`
  reproduces `dec eax; js` guards; a repeated "find slot" loop across four functions is one inlined `FindSlot`; branch order
  (`if (p) {...} else {...}` vs null-first) decides block layout. Still unmatched: 0x1000da40 (float).
- 2026-11-16 compiler finding: **/GS proves cl 13.00 (VS .NET 2002).** The original prologue is `mov eax,cookie; xor eax,[esp+N]`
  (cookie XOR return address) and the epilogue tail-jumps to `__security_check_cookie`. VS2003 SP1 (13.10.6030) emits
  `xor eax,esp` + call; Toolkit 2003 (13.10.3052) emits no xor + call, otherwise the same schedule as the original.
  Consequence: every function with a local array (/GS-protected) is unmatchable with 13.10. Toolkit and SP1 give identical
  results on all other functions so far. Flags now `/O2 /Ob1 /Gy /GS` (`/Ob1` proven: non-`inline` callees are not inlined).
  Status: 113 matched, 10 open (`decomp/open.tsv`: 13.00 codegen/encoder/GS walls). A cl 13.00.9466 would clear most of them.
- Recurring cl 13.00 walls (see decomp/open.tsv): **any [esp] memory operand is encoded with a zero disp8** (`89 44 24 00` vs VS2003 `89 04 24`), so every function that touches a local at `[esp+0]` differs by one byte per access; /GS prologue/epilogue; occasional register/schedule choices. 13.10 cannot fix these from source. Current: ~157 matched, 25 open.
- **Callee register knowledge inside one file** (corrected 2026-11-17): the caller keeps a value in edx across a call
  when the callee is *defined earlier in the same translation unit* and the compiler can see it leaves edx alone.
  This is ordinary cl 13.00 behaviour, not /GL. Place such functions after their callee in one file: the Vec4
  operators next to `Vec4()` in vec4.cpp (0x101ac9e0..0x101acbc0), KeyBank's constructor there too (0x101af420),
  Node::M_1019d6a0 after M_1019d680 and Node::M_101a2370 after M_101944d0 in scene_node.cpp.
  KeyBank::Sample (0x101af4c0) only gets `this` in edx when it is in vec4.cpp after Vec4() and Spline: its Vec4
  temporary's constructor and the Spline calls must both be known to leave edx alone.
- **Header declarations can change unrelated code** (2026-11-22): adding a member function declaration to KeyBank
  (key_track.h) changed cl 13.00's output for FUN_100b4f00 in char_move.cpp, which only includes the header. A
  `#define` in the same header did not. Run build_all + check_matches after every header change, however small.
- **/GX (C++ EH) is on in the original**: destructors with EH state frames match byte for byte with `/GX` and nothing regresses, so flags are now `/O2 /Ob1 /Gy /GS /GX`.
- Functions with an EH frame (`push -1 / push handler / fs:[0]`) must be written with **C++ linkage**: under /GX an `extern \"C\"` function is assumed not to throw and gets no frame. Map the mangled name to the address in `decomp/obj_symbols.tsv`.
- Another recurring 13.00 habit: when 0 is stored several times it is kept in a callee-saved register (`xor ebx,ebx`); VS2003 stores immediates. Seen in the enemy AI updaters 0x100e2080.. (source otherwise identical).
- At least one translation unit around 0x1013e6f0 was compiled **without optimisation** (/Od: ebp frames, every variable in memory, int3 padding between functions). VS2003 /Od lays the locals out differently, so those stay open for now (decomp/src_od/).
- 2026-11-16 status: 240 matched, 4 equivalent, 27 open of 1,212 game functions. All small (<=80 bytes) non-float functions are done; the open list is dominated by cl 13.00 walls (zero-register reuse, [esp] encoding, /GS, IPRA, /Od TU).
- cl 13.00 unrolls simple counted loops by 8 where VS2003 unrolls by 4 (0x101619a0).
- **Shared epilogue vs duplicated tails:** "VS2003 duplicates the return tail" was often a source spelling, not a wall.
  `if (g != 0 && (w = Get()) != 0) { ... } return 0;` gives the original's single epilogue (0x10001a90), and
  `if (g == 0) return 0; w = Get(); if (w != 0) { ...; return x; } return 0;` gives its `xor eax,eax` exit block
  (0x10001ba0, 0x10001bf0). Retry the open "tail duplication" entries with these forms.
- Anti-tamper stat guards decoded and matched (`src/anti_tamper.cpp`; see MODDING_MAP "Anti-cheat"); the checker
  0x1000e870 is 5 bytes off (edi/ebx swap in one loop). Counter 0x104c034c is a `short`, frame counter 0x104bf890 unsigned.
- **"Constant kept in a register" is not always a cl 13.00 habit.** VS2003 also loads the constant once (`mov eax,1` /
  `xor eax,eax`) when it fully unrolls a short constant-count loop or expands `memset`; it stores immediates only for
  separately written assignments. 0x100235e0 matched once its fills were written as `memset` and a `for` loop.
  Retry the open "zero/1.0f in a register" entries with loops.
- EH state stores that the original leaves out before a call (0x101728c0: no state update before `ModelState()`)
  match when that callee is declared `throw()`. The original compiler evidently knew the callee could not throw,
  probably because the callee was defined in the same file (see the callee note above). Use `throw()` on such
  declarations only where the
  original shows the missing state store.
- Callee-saved push placement follows the early exits: a test written as its own `if (x == 0) return 0;` before the rest keeps the push after it (0x1015c9e0). Where the original pushes *before* the first test (0x100014b0, 0x10199be0) no spelling has been found yet.
- A loop-style alignment pad (`lea ebx,[ebx]`, or `jmp` over `lea esp,[esp]`) before a 16-aligned join point with no loop around it is the end label of an inlined helper. Menu_SaveLoad 0x10064630 has two after `if (DAT_104bf8d4 != 1) DAT_104bf8d8 = 0; DAT_104bf8d4 = 2;`, which is MessageWindow_SetTitle(0) (0x1000cb30, `include/message_window.h`) inlined and folded. The same body written by hand has no pads. Look for a small function whose constant-argument fold gives the padded sequence.

## 2026-11-17: the original compiler (VS .NET 2002, cl 13.00.9466)
- The user installed VS .NET 2002 at `E:\Visual StudioNET 2002\`; `cl` reports 13.00.9466 and `link` 7.00.9466, the
  build in the Rich header. `decomp/env.bat` now uses it by default (`GURUMIN_VC` overrides).
- With unchanged sources: 334 matched (VS2003: 325 + 5 EQUIV). Every EQUIV (the `[esp+0]` disp8 encoding) and the /GS
  cookie function (0x1015fe20) match, so **all 190 /GS functions (1.07 MB of the 1.72 MB of code) are now in reach**.
  Two functions regressed because their source had been bent for VS2003; the plain forms match (0x10004650:
  `while (--count >= 0)`; 0x100235e0: seventeen separate `= 1` stores instead of a loop, which 13.00 turns into `rep stosd`).
- **The original used `/Op`** (improve float consistency): with it, three more functions match and none regress
  (0x100114a0's 1.0f kept in a register, KeyTrack::Insert's float parameters copied with integer moves, KeyBank::Clear).
  int-to-float conversions are rounded through the parameter's own stack slot (`fild [esp+8]; fstp [esp+8]`).
  Flags are now `/O2 /Ob1 /Op /Gy /GS /GX`.
- **Revisit every VS2003-era wall with the plainest source.** Workarounds written for 13.10 now hurt: the lazy-create
  getter 0x1017b1c0 and the shared-tail functions 0x10047400 / 0x100158b0 match as a plain `if`/`else` followed by the
  common code. The unroll-by-8 searches (0x100119d0, 0x10011aa0, 0x10148410, 0x100020d0) match as plain `for` loops.
- The remaining `open.tsv` reasons were written against VS2003; read them as "differs", not as a diagnosis.
- Still open under 13.00: the enemy AI updaters (0 kept in ebx, also used as the switch base: `sub eax,ebx`), register
  ties of a few bytes (0x1015cac0, 0x10169e10, 0x101992f0, 0x10034c80), 0x1000da40 (only the reload of the target).
- Tools: `tools/variants.py <file> <n> [flags]` builds `#if V == k` variants; `tools/pick.py <file> <k>` keeps one;
  `tools/sbs.py <obj> <symbol>` prints ours and the original side by side. From Git Bash set `MSYS_NO_PATHCONV=1`,
  or `/Op`-style flags are rewritten into paths.

- Float code now matches routinely under 13.00 + /Op. Patterns: an int-to-float assignment written plainly
  (`global = value;`) and repeated in a branch gives the original's single rounded temporary (0x1000da40 family);
  `if (cond) continue;` reproduces the NaN-aware `test ah,5 / jnp` of a negated float compare (0x100123d0); accessing
  `array[index].field` directly instead of through a local reference or pointer gives `lea reg,[reg+addr]`
  where a local gives `add reg,addr` (0x10016500, 0x1001a390). math.h's inline `acosf`/`atan2f` were emitted
  out of line (0x10001f60, 0x10001f80, symbols `_acosf`/`_atan2f`).
- /GS functions match like any other under 13.00 (0x10169780, 0x1013e0c0, 0x1005cdc0).
- **Functions with an EH frame need C++ linkage.** Under /GX (= /EHsc) a function *defined* `extern "C"` is
  treated as unable to throw, so its EH frame disappears. Name it `FUN_xxxxxxxx` without `extern "C"`
  (compare_obj and build_all accept the mangled `?FUN_xxxxxxxx@@...` names); declare C++ callees the same way.
- GameFile (0x101ad9e0 ctor, 0x101b2570 dtor -> File_Close) is 0x28 bytes: `FILE *handle; char name[0x24];`
  (Save_LoadSlot's frame). Save_LoadSlot builds one and never uses it.
- `strcmp(word, "literal")` is inlined as `repe cmpsb` with the literal's length including the NUL (0x100364a0);
  `memcmp` with the same length becomes dword/word compares instead.
- Pointer loops that end with a signed `jl` against the end address come from index loops (`for (i = 0; i < n; i++)
  a[i] = ...`), not pointer loops (0x1014b1c0).
- Local arrays are laid out in declaration order from the top of the frame down: the one declared first sits higher
  (ItmScramble: `table` declared before `key`).
- Math library (0x101ac000-0x101ae000, src/vec_math.cpp): `sqrt`/`sin`/`cos` are calls under /Op (no fsqrt inlining);
  float arguments of an inline helper are copied to the stack in right-to-left argument order (`LengthSquared(x, y, z)`
  copies z, y, x). Chained assignments store right to left (`a = b = c = 0` stores c first), which explains odd store
  orders (0x101ae770). Stores otherwise keep source order, so write them in the original's order (0x101ae710).
- Vec4 operators that return by value (0x101ac9e0 +, 0x101acb00 *, 0x101acb60 /) keep `this` in edx across the
  `Vec4()` constructor call because `Vec4()` is defined in the same file (vec4.cpp); they all match there.
- An empty `switch` case still counts when the compiler picks the split value if its body is not empty in the source
  (WindowProc's `case WM_KEYDOWN:` with a dead `if`).
- /Od translation units (src_od/) are built with `/Od /Oi`: strcpy expands inline there (0x1013e6f0). build_all includes them.
- Functions called on freshly allocated memory without a null check are plain methods (`node->Init()`), not placement
  `new` (0x1000b9f0).
- **The compiler picks its own operand order for `+` and `*` of equal-weight terms**; source order and parentheses
  inside one expression do not steer it. When the original evaluates the "wrong" term first, the first term was a
  named temporary: `float cosY = cos(a) * y; float ny = cosY + sin(a) * x;` (Vec4::RotateZ/RotateY/RotateX
  0x101ac300/0x101ac3a0/0x101ac440). Plane::Distance (0x101ad240) still resists because a temporary adds a store there.
- **x87 compare forms decide `test ah,1`/`je` versus `test ah,5`/`jp`** (2026-11-22, box_overlap.cpp): `!(a < b && ...)`
  gives the NaN-aware `test ah,5; jp`; the original's `test ah,1; je` / `test ah,0x41; jnp` come from the plain
  complements written out, `a >= b || c <= d || ...` (BoxCylinder_Overlap 0x10192200). An `if (d < r)` whose
  original is `test ah,1; je` was written `if (!(d >= r))` (BoxSphere_Overlap 0x10192520).
- **A float used by only one branch but written as a statement before the branch is hoisted** (cl 13.00 shares
  the `fild` with the other branch): writing it inside the call arguments (`Spline((float)offset / (float)span, ...)`
  four times) keeps one evaluation after the integer setup, as the original does (KeyBank::Sample 0x101af4c0).
- **Statement order before a call schedules zero/constant stores**: in Number_Draw 0x10012990 `y--; advance = 0;`
  before the ten `widths[k] = ...` stores gives the original order; declaring/initialising `advance` first does not.
- **Commutative sums of two pointer fields** (`sphere->y + sphereOffset->y`): the load order is the compiler's,
  whatever the source order; in 0x10192520/0x10192c90 it differs from the original at some sites (open).
- `-z` as a multiplication operand keeps `fld z; fchs; fmulp` when written `(-z)` (RotateY/RotateX).
- A min/max assignment whose value is the variable itself (`x = 1 < x ? x : 1`) is folded away; when the original
  keeps the self-copy, the result went into a new local (`float clamped = ...`, Input::Rumble 0x10142700).
- Two `return f(..., a, ...)` / `return f(..., 0, ...)` calls in an if/else let the compiler share the pushes of the
  common arguments, including a pushed 0 that doubles as the second call's argument (Texture_Load 0x1001e1a0); a
  local or ternary does not.
- `char` padding arrays inside a local struct turn on the /GS cookie; pad local structs with `unsigned long` arrays
  (VideoSurface::CopyTo 0x1015c6a0). `DrawText` is a windows.h macro: do not use it as a method name.
- A float temporary that lands in a dead parameter's stack slot comes from an expression passed straight to a
  `const float &` parameter (`v * (1.0f / frames)`, 0x1000ec70) or from a local assigned after the parameter's
  last use (FrameBlend_Start 0x1013fb10); a named local computed earlier gets its own slot.
- Whether a callee is *defined* in the same translation unit changes stack-slot sharing: with Cross defined in
  vec_math.cpp, PlaneFromTriangle (0x101af360) gave the Cross return temporary its own slot (frame 0x10 too big);
  in vec_ops.cpp, where Cross is only declared, it matches. Use this as evidence of the original file split.
- Same-file knowledge also decides **stack-slot reuse**: when a local vector's address is passed to a function the
  compiler cannot see, the slot is never reused for later temporaries. With Vec3_Dot / Vec3_LengthSquared defined in
  vec4.cpp, the segment-distance functions (0x101acd90, 0x101af260) put their last temporary in the dead direction
  vector and match. A frame that is 0x10 too big is a hint to move the callees in.
- **Inline budget**: within one function cl 13.00 expands an inline method only a few times, then calls the
  out-of-line copy. Game::TestScriptFlag (0x100022b0, now inline in game.h) is expanded 3 times and called once in
  ForEachActiveTrigger (0x100552f0), expanded twice and called twice in RaiseGameOverEvents (0x10054a60). A known
  function called "sometimes" after identical inline code is an inline helper.
- Two short constant loops in a row get **fused** by cl 13.00 (stores interleaved); written-out stores keep the
  original order and let the compiler keep the constant in a register for the earlier loops too (EffectManager::Reset
  0x10184170, effect definitions 0x1016f900: the two five-entry -1 caches).
- A clamp that tests the upper bound twice (cmp 15 / test / cmp 15 again) is the classic `max(min(x, hi), lo)` macro
  pair with double evaluation; write the macros inline in the expression (0x10017fc0). `add reg, -5` instead of
  `sub reg, 5` came from the subtraction inside the macro argument.
- Texture_Load (0x1001e1a0) is also an inline helper: functions that load a fixed texture expand it, including
  three `strlen` loops over the literal (0x10050460). `include/texture_load.h` holds it as `LoadTexture`.
- **Jump threading** of character tests: with `char c = *text;` cl 13.00 knows after `c == '('` that `c != '"'`
  and jumps past the later tests (blocks moved, about 15 bytes longer). Writing `*text == ...` in every test leaves
  the threading out; the loads are still merged into one register later (ListBank::FindCase 0x100558d0,
  ListBank::FindLabel 0x10055aa0).
- **extern "C" means no EH frame**: under /GX cl 13.00 treats an `extern "C"` function as not throwing and drops the
  exception frame for its temporaries. A function whose temporary (a returned Matrix4) gets an fs:[0] frame and
  state stores is a C++ function (Map_SelectMovie 0x1001f270).
- **Float comparison spellings** (fcomp, then the flag test): `a <= b` gives `test ah, 41h / jnp`, `!(a > b)` gives
  `test ah, 41h / jne`; `a >= b` gives `test ah, 1`, while `a < b` used as a skip test gives `test ah, 5 / jnp`. Pick
  the spelling from the flag test (KeyPrompt_DrawAll 0x1000d6b0: `!(x > -100000)` and `if (z < 0.9 || ...) continue`).
- **Element pointer after the first test**: a loop that tests one field of `array[i]` and only then works on the
  entry takes the pointer inside the `if` (`if (strcmp(events[i].name, "TALK") == 0) { GameTrigger *event =
  &events[i]; ... }`). Taking it first biases the loop pointer to another field. This matched RunQueuedEvents,
  IsQueuedEventReady and the Raise*Events family (0x10054970..0x10054c60).
- **Separate early returns**: a chain of float tests that ends with the return-0 block on the fallthrough of the
  last compare (the hit code behind a `jne`) was written as one `if (...) return 0;` per test, not one `||` chain
  (Box_Overlap 0x10191fd0; Cylinder_Overlap 0x10192a00). Stack slots shared between successive temporaries come
  from block-scoped locals: `float r; { float dy = ...; float dx = ...; r = ...; }` (Cylinder_Overlap).
- **Sum of two rounded calls**: `float s = (float)fabs(a) + (float)fabs(b);` rounds the first result into the cast
  temporary and copies it (fld/fstp) to a spill slot before the second call; separate statements keep it in place
  (Ground_HeightAt 0x10016700). Which operand cl 13.00 evaluates first does not follow the source order there.
- **ListBank::ReportError is an inline member**: FindVariable (0x10037090) expands it once (printf, callback,
  FreeAll inline) and calls the out-of-line copy 0x100359c0 for its other error. Units that need the expansion
  declare it inline in their own ListBank view (listbank_vars.cpp); listbank.cpp keeps the out-of-line copy.
- **Prefer repeated element access to a pointer local**: `T *p = &table[i]; p->a ... p->b` changes register and
  load order; writing `table[i].a`, `table[i].b` lets cl make its own address register like the original
  (Node::ApplyMaterialColours 0x1019c560, Npc_WaitFacingAway 0x10045c30, face change 0x1000f360 closer).
  Not universal: Sound_Queue 0x10054550 keeps a pointer in the original.
- **Switch on a value the cases set**: when a switch sets a count (`models = 1` / `2`) that later selects the work,
  `if (models == 1) ... else if (models == 2)` gets jump-threaded into each case; a second `switch (models)` keeps
  the stores and the dec chain of the original, and a `default:` that skips the work is a `goto` past it
  (Char_AttachWeapon 0x1001a990).
- **A slot variable for scaled indexes**: Companion1/2_Update (0x100b0880, 0x100b0f00) index their word, dword,
  state and record arrays through `int slot = index + k;` (`[slot + 2]`, kept in a register) and their byte arrays
  as `[index + 3]` (folded into the displacement). Writing `index + 3` everywhere folds all of them.
- **A zero kept in ebp means too many zero stores**: when ours holds 0 in ebp (`xor ebp,ebp`, `push ebp`, `mov [..],ebp`)
  and the original uses immediates, look for a store repeated before each `return`. Npc_WatchForPlayer (0x100b1520)
  clears `DAT_10210b30[index]` once at the end of an if/else-if chain; the compiler duplicates the epilogue itself.
- **An inline helper the compiler will not expand**: cl 13.00 sometimes emits a call to a small inline helper
  (`Atan2`, math.h `atan2f`) instead of expanding it, for reasons not tied to function size or position in the
  file. Where the original has the expanded code, call the library function directly: Enemy_ShoveAndBlast
  (0x10098be0) matches with `(float)atan2(x, y)`. Npc_RaiseAlarm shows the reverse (the original calls atan2f).
- **Register ties follow the struct's member list**: adding any member (even a pad split) to char_move.cpp's
  CharRecord flips Npc_PopUp (0x100b4f00) from matching to an ebx/ebp swap, while extra extern declarations do
  not. A function that needs another field of a shared view can go into its own unit with its own view
  (npc_sleep.cpp for Npc_SleepUntilWoken) instead of changing the view other matches depend on.
- **`dec / je / dec / jne` for "1 or 2"**: a switch whose cases 1 and 2 each carry the same statement
  (Enemy_DropIn 0x10048e90). Shared case labels compile to a range check, `x == 1 || x == 2` to two compares.
- **`sqrtf` and `atan2f` from math.h**: where `(float)sqrt(x)` stores its result after a following compare
  and `Atan2(...)` keeps the result in a temporary, the inline math.h float versions give the original order
  (Npc_RideBehindMarker 0x10047a10, Npc_RideAlarm 0x100b4930).
- **A float argument chosen by a condition**: where the original pushes the value straight from the FPU into one
  shared call, write two calls in if/else (the compiler tail-merges them). A ternary argument, or assigning a
  variable first, goes through a stack temporary (Shaga_Create 0x100425f0, Npc_Held 0x100469d0).
- **Calls cross-jumped from both branches**: when both arms end in the same calls (the pushes merge at a `jmp`)
  and the argument pops of three calls come as one `add esp`, repeat the last call in each arm instead of writing
  it once after the if/else; the shared form pushes the saved registers too early (TextureObject::DrawFrameAlt
  0x1014e940).
- **A walk up a linked list can be tail recursion** (Node::Root 0x101992f0): `if (parent == 0) return this; return parent->Root();` compiles to the original loop with the next pointer in edx; the plain `while (node->parent) node = node->parent;` loop keeps it in ecx. Try the recursive form when a pointer-walking loop matches except for its register.
- **A fallback object written as two calls**: when a node pointer falls back to another (`fire`, else the head) and
  the original loads the fallback into the same register before one shared call whose pops are not merged with the
  next call, write `if (a) Call(a); else if (b) Call(b); else {...}`: the calls are cross-jumped. The `a || (a = b)`
  spelling keeps the fail block inline and shifts registers (Enemy_SpinFire 0x100a7db0).
- **Vertex loops**: the loop pointer starts at the second field the body writes (DrawFrameAlt writes x then y; the
  original pointer is &y).
- **Early returns placed in the middle**: when the `return 0` block sits right after the guard tests (before the body)
  instead of being shared at the end, each test was its own `if (...) return 0;` (ModelBank::DrawLensFlares
  0x10166520, ModelBank::DrawShadows 0x10165e00).
- **Initialised local tables**: the order the compiler writes repeated values depends on the declaration order of the
  tables, which is not simply their stack order; try permutations (DrawLensFlares: sizes, steps, lenses, alphas).

## Source style (2026-11-17)
- Comment generously: a header comment per file, and a comment above every function saying what it does, who calls
  it and what its fields, constants and magic numbers mean. Comments never change the compiled bytes. The user's
  rule: "you can never have too many code comments." `decomp/QUEUE.md` section 7 lists files still missing them;
  `tools/annotate.py <notes.py>` inserts comment blocks above given signatures.
- Prefer readable names over placeholders once a function's role is clear; map non-address names to their address in
  `obj_symbols.tsv`, then run `tools/sync_symbols.py` to copy them into `symbols.tsv`.
- Some engine functions were written in inline assembly (frame pointer plus `or reg,reg` tests a compiler never
  emits): Mmv_DecodeFrameRle 0x10160a20 matches as an `__asm` block. Locals go in declaration order below ebp; label
  placement must follow the original jumps exactly. Do not name variables used inside `__asm` `length`, `size` or
  `type` (MASM operators).
- Two separate early returns (`if (!p) return x; if (!*p) return x;`) and a combined `||` condition give different
  block layouts (Node::Find 0x10191a30 needs the separate form).
- Call arguments that are themselves calls are evaluated before the callee's `this` is loaded only when written
  into a local first (TextureObject::BindStage0 0x1014b670).
- Same object, several names: ListBank (listbank.cpp), ResourceBanks (resource_banks.cpp) and TokenReader
  (type_parse.cpp) are one class (the script parser and its buffer banks); SceneOwner (texture_mgr.cpp) is the
  CharModel inside Character, and its `scene` is the Playback object (model_state.cpp). Merge them into shared headers
  (QUEUE section 8).
- A pointer chosen in an if-chain and used once afterwards (`p = &a[i].f; ... if (*p > 0) (*p)--;`) gives
  `add reg, symbol`; the original's `lea reg, [reg + symbol]` into a shared tail comes from repeating the whole
  statement in every branch and letting the compiler merge the tails (0x1001d7d0).
- Sums of products are reordered by the compiler regardless of source order; only the grouping (parentheses)
  steers the evaluation order (Plane::IntersectRay 0x101ad2c0: y + (x + z)).
- A search loop whose found-branch only `break`s is unrolled by 8; putting the work inside the loop and returning
  from there keeps the simple loop (ModelBank::Add 0x1017c570).
- A function's stack frame size reveals the type of its locals: three floats computed into a 16-byte constructor-less
  local is a `float t[4]` (TransformNormal 0x101ae130), not a Vec3 or separate floats.
- TextureObject +0x08/+0x0c are the texture width and height in pixels (FUN_101616e0 divides pixel UVs by them).
- Exception frames: with /GX (/EHsc) an `extern "C"` function gets no unwind frame for its locals with destructors.
  A function whose original has `push -1 / push handler / fs:[0]` must be a C++ function (Point_Transform 0x1001c510).
- Fully unrolled copies (28 byte stores, 5 dword stores) are written out in the source; cl 13.00 does not unroll a
  loop that long (DemoRecording::PlayFrame/RecordFrame 0x1015cf30/0x1015cd70).
- `x = (a + b) * c` with a store between the add and the multiply was two statements: `x = a + b; x *= c;`
  (Region_IsVisible 0x10021070). A float rounded to memory before fabs/atan2 means an inline float helper
  (AbsFloat in char_leash.cpp, Atan2 in enemy_moves.cpp), whose parameters are spilled.
- An `else` block placed first (right after a test) usually comes from else-if chains with one shared `return` at
  the end rather than early returns (ListBank::Evaluate 0x100caf90).
- A function that never uses `this` but is called with ecx set is a member function (TextureManager::OnLostDevice
  = Render_OnLostDevice); one called without ecx set from a method is a free or static function.
- Pass-through field compares: `0.0f < x ? x : 0` and `1.0f > a ? a : 1.0f` keep the original operand order of the
  compare; the mirrored spelling flips the fcomp operands.
- One helper applied twice to the same expression (`SignedField(code % 10000)` in a test, then again in the use):
  write the expression twice. CSE shares the idiv, the first inline call works on a copy and the second folds the
  dead remainder in place. A named `cost` local gives lea forms instead (Map_RouteLength 0x10049990).
- A pointer local into a table (`RoutePoint *point = &table[i]`) changes the order of the induction increments at
  the loop end; indexing the table directly can restore it (Map_RouteLength).
- A field read twice in a condition (`s[t].DistanceSquaredToLine(a, b) > s[t].w * s[t].w`) was written that way:
  the radius is copied to a stack slot before the call and the array address is computed twice, which keeps
  the triangle offset in a register. A named `float radius = s[t].w;` local CSEs the address and spills the
  induction variables differently (Node::SegmentHitsMesh 0x101a8780).
- A per-vertex index read once into an `int vertex` local and five `vertices[vertex].field` reads give the
  original's single imul per corner with the vertex pointer reloaded each time (Node::SegmentHitsMesh).
- `x = total + f(); return x;` against `total += f(); return total;`: the second rounds through total's own stack
  slot after the register pops.
- Arrays read at `index + 3` with scaled addressing (shorts, ints, 0x188 records) keep `index + 3` in a register only
  when it is a named local; byte arrays fold it into the displacement either way (Char_ClampStatus 0x1001a840).
- Tokenizer loops (ListBank::ReadWord and siblings 0x10035b10-0x10035fa0): test the characters through `*p` every
  time, not through a `char c = *p` local; with the local cl 13.00 threads jumps the original does not. The length
  limit is the loop condition at the bottom (`do { if (error) return 0; ... } while (length < 0x100);`), and each
  exit keeps its own copy of the terminating stores and `return 1`.
- A range test compiled as `fcomp 1.0; test ah,0x41; je skip` then `fcomp 0.9; test ah,5; jnp skip` is the negated
  form `if (!(z > 1.0 || z < 0.9))`; `z <= 1.0 && z >= 0.9` (in either operand order) gives `jp`/`test ah,1`
  (Drops_Update 0x1000c390). A float argument computed into a register and pushed as an int (`mov edx,[tmp]; push
  edx`) was a named local (`float alpha = ...;`) rather than an expression in the call.
- Two branches ending in the same call can be one shared call in the original's code because the compiler merged
  them: write the call in each branch (`if (a) { s = 1; Play(); } else if (b) { s = 0; Play(); }`), not an early
  `return` plus one call at the end; the merged tail schedules `this` differently (Npc_TalkFacing 0x1001c210).
- Several full calls in an if/else chain that differ in one argument compile to shared pushes before the branch,
  one differing push per branch and one call (cross-jumping); a conditional expression in the argument gives a
  select into a register instead (Char_SetHandTool 0x10012510, Screenshot_ReadBackBuffer 0x1013e9d0). A global read
  in several conditions of the chain loaded early into a register was a local (`int level = DAT_10c64ad4;`).
- A mode that is first resolved (`if (mode == -1) { if (...) mode = 0; else mode = 1; }`) and then dispatched lets
  the compiler thread the jumps straight into the attach/remove blocks; a conditional expression gives setcc.
- A /GS cookie (`push ecx` frame, cookie check before every return) in a function without any buffer means the
  original declared a character array it never used; an unused `char unused[8];` reproduces it
  (Helmet_FaceSettings 0x10014d30, commented in the source).
- A search loop whose found-branch's work sits at the end of the function and jumps back to a shared tail, while
  the not-found work falls through after the loop, was written with the whole tail (`...; return slot;`) inside the
  found-branch and again after the loop; the compiler cross-jumps the duplicate tail (Node::Load 0x101ac010).
- A destination computed before a branch that only picks the source (`mov edi,dest; test; je; mov esi,a / lea
  esi,b; rep movsd`) was an if/else with one full assignment per branch; the compiler hoists the common part. The
  conditional-expression form computes the source first (Node::SetupSkins 0x101ab9d0).
- Arguments pushed before a branch that only computes one argument (the sprite alpha in EffectManager::DrawSpriteA
  0x101890f0) come from one full call per if/else branch: the compiler hoists the shared pushes above the branch
  and merges the tails. A conditional expression as the argument is evaluated before all pushes instead.
- Bits 0-7 of a word stored as bytes with `mov al,dl; shr al,n; and al,1` (byte shifts) were read from the low byte:
  `((unsigned char)bits >> n) & 1`; `(bits >> n) & 1` shifts the whole register (InputState::SetButtons 0x1015cae0).
- A `<=`/`>=` test that lets NaN through (`test ah,0x41; jnp` / `test ah,1; je`) is the negation of the opposite
  comparison, written as an early exit `if (a <= b || c >= d) return 0;` (Box_Overlap 0x10191fd0).
- Two Vec4 locals sharing one stack slot (a frame 16 bytes smaller than ours) were in disjoint scopes: the copy
  used only to measure a distance sat in its own block (Char_CanSee 0x10043e20).
- A call with ecx loaded from a global object pointer and no `add esp` afterwards is a method of that object
  (thiscall), even when the pointer was only tested before: `DAT_10255f08->Raycast(...)`.
- Locals that overlap the stack slots of temporaries from an earlier block (Vec4 locals at offsets the original
  also uses for the smoothing temporaries) were declared in their own `{ }` scope after that block; function-scope
  locals declared later still get slots of their own (Sound_UpdateListener 0x10021c80).
- A clamp computed twice (`test; setl; dec; and` before and after the compare) is a `__max` expression repeated
  in a conditional: `__max(0, t - 5) > limit ? limit : __max(0, t - 5)` (Item_DrawCaption 0x1003c370).
  `__max(0, x)` gives `setl`, `__max(x, 0)` gives `setle`.
- `strlen(s) / 2` divides unsigned (`shr`); the original's `cdq; sub; sar` is `(int)strlen(s) / 2`.
- Range checks written as two statements (`if (i < 0) return 0; if (i > max) return 0;`) keep the shared
  `return 0` block inline right after the tests; `if (i < 0 || i > max) return 0;` moves it to the end of the
  function (Bgm_Play 0x1003aef0).
- An if/else whose then-branch is the common path (`if (attacking == 1) {...} else {...}`) puts the else-branch at
  the end; the same logic as early returns lays the blocks out the other way (Kind4a_Update 0x1009ddd0).
- A reloaded-pointer copy `T *p = param;` made before a loop moves the parameter loads above the first test
  (Rle_Decode 0x101612b0); a table indexed `[group * 256 + word]` should be declared `[groups][256]` when the
  original steps a row pointer (Text_Substitute 0x1000c020).
- An `inline` helper with a call and three stores was not inlined at /Ob1 here; duplicated statements matched.
- `cmp eax, ebx` (the zero register) after a call where cl 13.00 gives `test eax, eax`: the result went into a
  local first (`void *opened = file.Open(...); if (opened == 0)`) (MmvPlayer::Open 0x10160a90).
- A zero register (`xor ebx, ebx` in the prologue, `push ebx` for every 0) can lose ebx to a CSE of an address
  used at the top and again deep in the function (`&DAT_10db6ee8[index].matrix`); reaching the second use
  through a local pointer (`AnimModel *model = &DAT_10db6ee8[index]; ... &model->matrix`) stops the CSE and
  brings the zero register (and the original frame size) back. Stores issued before the Vec4 constructor calls
  come from statements written before the Vec4 declarations (Enemy_ChaseAndSlash 0x1009d490).
- cl 13.00 cross-jumps identical tails of sibling if-blocks and canonicalises the order of plain constant
  stores while doing so: swapping two stores in the source does not change which blocks merge.
- A palette built with `movzx reg16` and `and reg, 0xfff8` works on unsigned shorts:
  `(unsigned short)(unsigned char)r & ~7` (MmvPlayer::Open).
- Reads from a buffer with a moving cursor whose position is also counted in an address-taken int (`p += n;
  offset += n;` per field) explain a run of stores to that int between the copies (Save_Load 0x1003b2c0).
- A big sparse `switch` lays out its case blocks in source order; recover that order from the original's block
  addresses (Map_SetEntryFlags 0x10021380).
- cl 13.00 turns plain fill loops into `rep stosd`, packing shorts into dwords (`for (i < 0x44c) a[i] = -11` gives
  `mov eax, 0xfff5fff5; mov ecx, 0x226; rep stosd`), so a fill of a value memset cannot make is still a loop
  (Game_Reset 0x1003e710).
- Ghidra drops `this` for thiscall callees whose ecx is set well before the call (`mov ecx, 0x104687b8` six
  stores earlier): an unexplained early `mov ecx, imm` is the object of the next call.
- Long store-only functions can be generated from the pseudocode (store widths from the asm, `Fill` targets from
  the `mov ecx` before each call) and then fixed by comparing instruction offsets to find the first divergence.
- A local `char table[256][4] = { "..", ... }` compiles to a word load and a byte load from each literal plus a
  zero store for the 4th byte (no block copy); replay the stores against the image to recover the strings
  (Text_ToFullWidth 0x1015d520). Test order matters: `(c >= 0x7f && c < 0xa0) || c >= 0xe0` is not the same layout
  as its negation with the branches swapped.
- `(flags & 0x80) == 0x80` on an unsigned value compiles to `test al, al / jns`; on an `int` it stays
  `and ecx, 0x80 / cmp cl, 0x80` (Ability_GetDescription 0x100144c0, whose parameter is therefore `int`).
- A loop that clears two arrays of the same length (`for (i < 0x42) { a[i] = 0; ...rest... }`) is split by cl:
  the plain clear becomes a `rep stosd` scheduled into the code before the loop, which changes the loop's registers;
  a separate `memset` before the loop does not give the same code (Map_ResetOnEnter 0x10023900).
- Small local char buffers: the declared size changes the frame layout. `char number[8]` (only five bytes used)
  matched WorldRenderer::LoadCharacter where `[5]`/`[6]` put it below another local.
- Structures with many member arrays (WorldRenderer, 0x1410d0 bytes) are easiest to lay out with a generator that
  takes (offset, size, declaration) rows and emits the padding; a constructor that constructs the members in order
  checks the layout (world_ctor.cpp).
- Texture loads by literal name (`FUN_10174ce0(FUN_1013f8a0(name, 0), 0, 0)`) come from a one-line inline helper
  (`LoadIti`, face_anim.cpp / bs003_variant.cpp); the full `LoadTexture` of texture_load.h is not inlined at /Ob1.
  Repeated loads at the end of switch cases are cross-jumped into one call, so write each case in full.
- Locals that live only in one block should be declared in that block: cl reuses their stack slot for later
  spills (SystemSav_ReadRecord 0x101603c0 matched its frame once `zero`/`word` moved into their loops).
- A triangle test whose later Vec4 locals share stack slots with earlier call temporaries (and whose shared
  `return 0` sits at the end) nests the rest inside `if (!(u < 0.0f) && !(u > det)) { ... }` (C0/C3 tests, NaN
  passes); each successive interpolated value got a fresh local (`edgeU`, then `hitU`): reassigning `uB` gives
  the same instructions with the float slots permuted (Node::IntersectTriangle 0x1019dca0).
- `DAT_11affdd8[name[3] * 10 + name[4]]` was `DAT_11afffe8[(name[3] - '0') * 10 + (name[4] - '0')]`: cl folds the
  '0' digits into the displacement (0x11afffe8 - 0x210), so a fake global appears 0x210 below the real table.
- Rule merge trap: reading a global array through a struct that has any union or nested POD struct member (even
  `union { int a; float b; } u;` far from the fields used) changed register allocation in Npc_Greet 0x100b8220
  (an extra `push ebx` and a CSE of `index * 4`); the same offsets in a struct whose members are only scalars,
  arrays, pointers and Matrix4 match. AnimModel has four anonymous unions, so npc_greet.cpp keeps its Character view.
- Three FILETIMEs in ascending address order with an 8-byte gap after the last-write time are an array
  `FILETIME times[4]` used as times[0..2] (SystemSav_WriteRecord 0x1015fea0); separate variables are ordered differently.
- Copying a byte buffer once per loop iteration into an `int c` local gives the original's movsx/xor/add sequence
  in cl's 8x-unrolled checksum loops; two reads of `name[i]` do not.
- Comparison operand order is visible: `count > to` gives `cmp eax(count), reg / jle`, `to < count` gives
  `cmp reg, [count] / jge` (Node::BlendFrames 0x1019b630). Matrices passed by value are copied with `rep movsd`
  onto the stack in argument order.
- Reading the three triangle indices into locals once, and evaluating `MAX(...) + MIN(...)` in that order, matched
  the original's load and compare sequence in Node::BuildCollisionSpheres 0x1019e120 (MIN/MAX macros evaluate twice).
- A setter that loads the stored value before the index (`mov eax, [esp+4]; mov edx, [esp+8]`) when its twin loads
  the index first stores a float: float copies go through integer registers in argument order (0x1015cac0).
- Walls written down as "VS2003 does X" deserve a retry with cl 13.00: statement order of independent stores
  does change the output (0x10034c80 matched with the two stores swapped, 0x10166870 with `m_00b8 = 0` first),
  and a call that keeps `this` in ecx is a method call (MeshInstance::M_10169e10 calls Upload, not a free function).
- `int left = counter; if (left != 0) counter = left - 1;` gives `dec eax / mov [counter], eax` where
  `counter--` copies to another register first (0x1000ed20). A clamp written as two ternaries through a second
  variable (`int c = x > 0 ? x : 0; int slot = c > 3 ? 3 : c;`) gives the branchless low clamp and a
  `mov esi, 3 / cmp / jg / mov esi, eax` high clamp (0x1016b1e0).

## 2026-11-22: one shared Vec4 (include/vec4.h)
- Replacing the per-file Vec4 copies with `include/vec4.h` (out-of-line `Vec4()` at 0x101ac990, so Vec4 is non-POD)
  changed no codegen in 109 files, including the 27 whose copies had no constructor: uninitialised `Vec4` locals,
  Vec4 arrays and extern Vec4 globals do not pull in a constructor call there. Reference vs pointer parameters
  (`DistanceToSegment`) and `const` on `operator*` change only the mangled name.
- Exception: `npc_greet.cpp` Npc_Greet (0x100b8220). With the shared header and no other change the compiler keeps
  `&DAT_10254390[index]` in ebx (extra push, frame +4). The member set or order of the declaration tips register
  allocation, so declaration order can matter even for out-of-line methods (open.tsv).
- A union member cannot have a constructor (C2620): `scene_node.cpp`'s `bounds` union became plain `Vec4 bounds[3]`.
  The int fields that overlaid it were floats (first vertex z); a float-to-float field copy compiles to the same
  `mov` pair as the int copy did.
- `char_slot.h` `ParamVector::operator=` sits at 0x101acc20 (Vec4::Copy), and the implicit copy assignment
  0x100357a0 calls it per member. This suggests Vec4::Copy is really `Vec4 &operator=(const Vec4 &)`. Unconfirmed,
  because no source file uses ParamVector yet.

## 2026-11-22: one shared Matrix4 (include/matrix4.h)
- `include/matrix4.h` replaced every per-file `Matrix4`/`Matrix`/`CharMatrix`/`NodeMatrix` copy (77 files plus
  `world.h`) with no codegen change, including about 45 files whose copies had no constructor or destructor: giving
  them `Matrix4()` (0x101ae770) and `~Matrix4()` (0x1013eda0) changed nothing, because they only use matrices through
  pointers, references and members of records they never construct.
- `SetTranslation` takes the `Vec4` its callers pass by value (16 bytes on the stack, the same as four floats); the
  definer in containers.cpp matches with either spelling. `SetPosition(x, y, z)` at 0x101adec0 in map_view.cpp is
  `Translate`.
- A class with a constructor cannot sit in a union (C2620), so the `Character` position union in world.h (and the
  one in pushing_platform.cpp) became plain `Matrix4 matrix`, and the position is `matrix._41/_42/_43` through the
  D3DMATRIX-style element names the header adds beside `m[16]`. The camera list `DAT_1032d5b8` stores `Vec3 *`, so
  `&matrix._41` is cast there until Vec3 is shared.
- `node_dtor.cpp` `NodeBody::~NodeBody` destroys Matrix4 members with the shared empty destructor; its other member
  type `Element` (float v[4] with an out-of-line destructor) looks like Vec4 with an empty destructor. Unconfirmed.

## 2026-11-22: Vec3, Node and the COM/D3D views shared
- `include/vec3.h` is a plain POD (x, y, z). The `Vec3_Dot`/`Vec3_LengthSquared`/`Vec3_Length` helpers take `Vec4`
  pointers at every call site (vec4.cpp defines them that way), so they are declared once in `vec4.h`. `Matrix4` has
  a third union view of its rows as `Vec3 axisX/axisY/axisZ/translation` (POD, so allowed in the union); the camera
  list `DAT_1032d5b8` takes `&matrix.translation` without a cast.
- `include/node.h` is the one `Node` (0x18a0 bytes); 45 files plus nine aliases (RootNode, HeadNode, PathNode,
  AnimNode, CollisionNode, PlacedNode, ShagaNode, ModelNode, VisNode) use it, and world.h's `m_22fc`/`headNode` are
  `Node *`. Facts the merge settled:
  - `QueueDraw` (0x10199e20) takes the parent matrix first: `DAT_10836ec8` is a Matrix4 (game_reset.cpp sets it to
    identity), not a draw list.
  - `M_101946b0` takes an `unsigned char` (its callers push 0xfe, not -2) and sets `unsigned char m_189a`;
    `m_189a`/`m_189b` must be unsigned or Init's 0xff stores become `or al, 0xff`.
  - `M_10191920` collects up to 8 nodes in state 0x4000 and `M_10191990` stores them in the saved block
    (`NodeSaved::linkedNodes`, Node pointers).
  - `RestartColourPlugs` (0x1019b300) passes the motion name through to the keys (`const char *motion`).
  - Copying the float fields 0x1880/0x1888 with `=` compiles to integer moves, so one typed field set serves both
    Init (float stores) and Clone (copies).
  - `Node::tint` (0x2ac) w is the alpha node_fade.cpp fades; `pathKind` (0x1879) = 2 marks camera paths.
- `include/com_min.h` holds `IUnknownLike` (QueryInterface takes `const _GUID &`), `include/ddraw_min.h` the full
  `SurfaceDesc` and `IDirectDrawSurfaceLike`; `d3d9_min.h` gained `LockedRect`, the texture's typed
  GetSurfaceLevel/LockRect/UnlockRect slots and `IDirect3DSurface9Like` (slots named after the real API).
- `CharRecord` (`DAT_10db6ee8`) is the AnimModel embedded at `Character + 0x10` (`DAT_10db6ed8`): merging it means
  sharing AnimModel (anim_model.cpp, `CharModel` in world.h) as `Character::model` first. A field at CharRecord
  offset o is at Character offset o + 0x10.

## 2026-11-22: node data, mesh, key tracks, AnimModel and DirectInput shared
- New headers: `key_track.h` (KeyTrack 0x14, KeyBank 0xa84), `node_data.h` (NodeExtra, NodeSkin 0x561c, NodeSaved),
  `mesh.h` (Vertex 0x70, VertexPack 0x74, IndexList, Material 0x180, MeshData), `anim_model.h` (AnimModel 0x249c,
  MotionChannels 0xa98, Playback 0x7c84), `dinput_min.h` (DirectInput interfaces, effect structs, Input).
- **AnimModel is 0x249c bytes, the full record stride.** So a Character record cannot hold one at +0x10: the old
  `Character` view (`DAT_10db6ed8`) was the same array seen 0x10 bytes early. Its `id` (0x0) is the previous slot's
  `AnimModel::nextSlotId` (0x248c), and its 0x4-0x10 bytes are the previous model's 0x2490-0x249c. All users now
  index `DAT_10db6ee8` (field offset - 0x10); a record's id is read as `DAT_10db6ee8[i - 1].nextSlotId`.
- Classes with constructors (Vec4, Matrix4, KeyBank, KeyTrack) cannot sit in unions (C2620), so AnimModel's
  multi-use slots are anonymous unions/structs of PODs only. model_params.cpp stores an int mode in the float
  `m_23d8.z` through `*(int *)&m_23d8.z` (the original writes the int bits there).
- Codegen-neutral changes tested: float stores of 0 match int stores; float parameters passed through match int
  parameters (SetPendingPush, M_10161af0); KeyBank needs no destructor.

## 2026-11-22: behaviour state, effects, textures, model bank, script parser shared

- `DAT_1020b430` is one 0x40-byte behaviour state per character slot (`char_state.h` CharState): each behaviour
  reads the same words under its own names, so the header gives each offset a union of the names (with the file
  that uses each). Whole-block users index `words[16]`; char_shove.cpp sees four `Shove` entries.
- **/GS cookie from a struct's char array:** a local of a struct type that contains a `char` array gets a security
  cookie. EffectOptions' locals (npc_frozen.cpp, char_move.cpp) have one in the original, so the type has a char
  array; `settings[8]` at 0x24 is the one consistent with every use. Without it the prologue lacks the cookie.
- **throw() on a constructor changes the callers' EH states:** EffectOptions() is declared `throw()`; without it the
  caller emits an EH state store before the call (anim_channels.cpp ModelInstance constructor).
- An inline member defined in the class body can still be called out of line where the original did:
  Game::TestScriptFlag (0x100022b0) stays inline in game.h and item_caption.cpp, route_follow.cpp, damage_zone.cpp
  and tutorial_page_next.cpp still call (and emit) the out-of-line copy.
- Bank models are AnimModels (0x249c); the bank's `Add` stores the model's root node twice (root, loadedRoot).
- ListBank's 0x440c table holds a constant's value or a pointer to a variable's storage (`ScriptVariableValue`
  union), and 0x2240c is the evaluation result buffer.
- **Allocate-if-missing through a local:** `Playback *playback = motion; if (playback == 0) { motion = alloc(...);
  playback = motion; } playback->Init();` keeps the member in ecx (the `this` of Init) as the original does
  (AnimModel::CloneModel/LoadFile, 0x1017b210/0x1017b9b0); testing the member directly reloads it into eax, and
  `motion = playback = alloc` keeps eax too.
- A 4-element float row zeroed with a `for` loop (`DAT_10255160[index][k] = 0`) is unrolled into stores of a zero
  register shared with the next loop's counter; four separate `= 0` statements use the callee-saved zero instead
  (Enemy_SeekShaga 0x100d2110).
- `static const float` constants are read from memory (`push dword ptr [x]`, `fcomp [0]` relocations to our own
  data); the original's immediates and its .rdata float constants come from `#define` literals (shadow_texture.cpp).
- cl 13.00 unrolls a counted loop over a local table 8 times by itself (`for (m = 0; m < 0x2000; m++) if (count[m])
  {...}` in Node::BuildMesh 0x101a82d0 gives eight bodies and `add eax, 0x20`), and turns zeroing loops over local
  int tables into `rep stosd`; a 0x28000-byte frame of int tables gets the aligned `and esp, -8` prologue.
- **atan2 spellings decide the frame** (enemy_strafe.cpp): `(float)atan2((double)x, (double)y) + 3.1415f` gives the
  aligned `and esp, -8` frame and rounds the result (`fstp`/`fld` of a float slot) right after the call; the float
  `Atan2(float, float)` helper rounds the same way but leaves the frame unaligned (ebp then becomes a fourth register),
  and helpers with double parameters (or `atan2(float, float)` from <math.h>) align the frame but drop the rounding.
- `cond ? "a" : "b"` as a call argument is evaluated before the other arguments; the original's `push` of the common
  arguments followed by a `jne` between two string pushes comes from an if/else with two full calls (Enemy_StrafeAndBurst).
- Look-alike functions match from one source shape: Enemy_StrafeAndLunge 0x10099040 matched at the first build from
  the draft of its twin 0x1009a690 with only the differing lines changed.
- Copying a Vec4 from a pointer next to setting another Vec4's fields: the stores interleave as in the original
  when the other vector is written first (`up.x = 0; up.y = 1; up.z = 0;` then `caster.x = p->x; ...`; Node::
  RenderShadowTexture / DrawShadow).
- Assigning the address of an operator temporary to the pointer
  parameter (`colour = &(*colour * tint);`) compiles (C4238) and gives QueueDraw's frame and parameter reuse (QueueDraw itself is still open).
- Matrix product temporaries of `projection = projection * (if/else ScaleMatrix(...))` share one slot when written
  as an if/else with the product in each arm (two EH states, one tail); a ternary is not needed.
- **Three-term float sums inside call arguments** (WorldRenderer::DrawPictures 0x10167440): with the half size CSE'd, `x - half + offset` and `y + half + offset` came out in the original order, but the right edge (original `fld offset; fadd x; fadd half`) needed the grouping `offset + (x + half)`; the same spelling in another branch may still schedule differently, so try groupings per call. A `Vertex *first = &v[i]` pointer local gives `add esi, edx` where the original keeps the index product and uses `lea` twice: index the array again instead (Node::BuildMesh 0x101a82d0).

## Stack-slot order of same-size locals (2026-11-23)
cl 13.00 does not place same-size locals (two Vec4s, two ints) in declaration order: the variable referenced more
often gets the lower address; declaration order only changes the constructor call order. Two locals with swapped
slots therefore usually mean a reference count is off, often a statement that uses the wrong variable
(Sound_UpdateDopplerLoops 0x1004fbe0 matched once step.Fill(0) became lead.Fill(0), which is also what the code
means). A self-assignment does not count. Char_HikariRide 0x1002cc70 (ints) does not follow the rule yet.
Ints follow the same rule counted after optimisation: memory references that survive (address-takes included,
CSE-merged source repeats not), cold blocks not discounted, ties to the second-declared variable; the fild spill of
an int-to-float conversion counts as a reference of its variable. Block-scoped one-use variables may share a slot
with a later block temporary. Char_HikariRide: eight real &next references would flip next/group; the source the
original had there is not found yet.

- Anonymous unions added to Node (m_1884) or AnimModel (m_0280) changed the code of unrelated matched functions (Node::WeldVertices, FUN_100b4f00 in char_move.cpp): give a reused field one plain typed member (rename it) or split a pad into typed members instead of wrapping it in a union in these classes. Adding an #include to char_equipment.cpp also changes FUN_100615f0 (20 -> 349 diffs), so that file keeps local declarations.
- FUN_100b4f00 (char_move.cpp) is a fragile ebx/ebp register tie: the number of declarations in matrix4.h tips it (one extra member or free declaration loses the match, two restore it). When a new Matrix4 member breaks it, also move a genuinely shared declaration into matrix4.h (batch 344 moved DirectionMatrix and Multiply there), never a dummy or duplicate.

- Effect lights: DAT_118e99f8 has EFFECT_LIGHT_COUNT (0x80) manager lights plus two ribbon lights (0x80, 0x81) that
  EffectManager::DrawRecord builds; Init/Reset clear only the first 0x80 (rep stosd 0x80280 dwords), so they spell the size
  as sizeof(EffectLight) * EFFECT_LIGHT_COUNT. The old globals DAT_11aea3f8/DAT_11aeabfc are light 0x80 fields
  (pointCount, points[RIBBON_JITTER_BASE + i]). Resizing a shared array changes every sizeof() of it in matched code.

## Linkage and EH frames (0x1011d810, batch 373)
An `extern "C"` function built here under /GX got no EH frame (no `push -1 / handler / fs:[0]` prologue and no
`mov [esp+N], state` writes around temporaries with destructors). When the original has that prologue, give the
function C++ linkage and map its mangled name in obj_symbols.tsv (in-game frame: `?FUN_1011d810@@YAHXZ`). Check the
other huge drafts (Bs00x_Update, DrawLit, RunScriptFrames) for the same gap.

## Zero register (cl 13.00, batch 382 experiments)
- cl 13.00 keeps 0 in a callee-saved register (ebx/edi/esi) only when zero is used on both sides of a call, about
  6-7 counted uses: stores of 0, `push 0`, and compares of a memory operand with 0. Ten zero stores in one call-free
  block stay immediates; compares of a value already in a register (`f(x) == 0`) do not count.
- With the register present, `x == 0` becomes `cmp reg,zreg` only when the compared value is live afterwards (a
  named local read later, a parameter, a value used in the else arm or returned); a dead temporary stays `test`.
- So an original `cmp eax,ebx` on a dead value points to a second use we have not reconstructed; ours hoisting a
  zero the original lacks means too many zero uses across a call. Tests: scratchpad w382z (z1..z8.cpp).

## Zero register: `cmp reg,zreg` needs a later use (batch 391)
- Once the zero register exists, `x == 0` on a dead call result is `test eax,eax`. It becomes `cmp eax,zreg` when
  the value is used again on a path after the compare, including uses that fold to a constant: cl knows `x == 0`
  inside the taken arm, so `if (r == 0) { ...; return r; }` compiles `return r` as `xor eax,eax` but still counts r
  as live (cmp); without the return it is test. An `else` arm that reads r counts too.
- RenderTarget_Probe matched this way: `int updated = ...UpdateImage(0); if (updated == 0) { ...; return updated; }`
  (the failure arms return the failed result).
- Do not invent a redundant use (`flag = (found == 0)` inside an arm where it is known true) to get the cmp; that is
  steering. Look for a real use: the result returned, stored or passed on.

## Vec4 is block-copyable; 0x101acc20 belongs to a second type (batch 383)
- Renaming Vec4::Copy (0x101acc20) to a user-declared `void Vec4::operator=(const Vec4 &)` (the original returns w
  in eax, not this) keeps CharSlot::operator= and FUN_1003e710, but every struct holding Vec4 members then copies
  member by member through the operator instead of rep movsd: Node::Clone (`*saved = *source->saved`, NodeSaved
  with Vec4 fields) and MeshData::UploadBones lose their matches.
- So the original Vec4 has no user-declared operator= (trivially copyable), while CharSlot's position has a type
  whose operator= is 0x101acc20: ParamVector in include/char_slot.h is most likely a real second original type.
  Keep it; do not merge it into Vec4. The rename was reverted (script and notes: scratchpad w383v).

## Stack-slot order of locals (cl 13.00, batches 381-384)
- cl 13.00 orders frame slots by reference count; locals with equal counts go later-declared = lower address.
  Many walls whose code matches but whose `[esp+..]` offsets differ are this (tools/sbs.py normalises offsets and
  hides them; scratchpad w381d/strict.py keeps them). Levers that worked: a local of its own instead of reusing one
  (Npc_RideBehindMarker), declaration order when counts are equal. When the original needs a reference-count order
  no plausible source gives, record it and move on.

## SIB base/index order (cl 13.00, batch 391 experiments)
- The source order of base and index is discarded: `p[i]`, `i[p]`, `*(p + i)`, `*(i + p)` and an inline accessor give
  the same `[eax+esi+N]`. Pointer vs array, signed vs unsigned index, local vs global base, member offset, loop
  direction and first use change nothing on their own.
- The order follows the function-wide register allocation: three identical haze loops in Menu_DrawScene come out
  `[eax+esi]`, `[eax+esi]`, `[esi+eax]`; the same block alone in a function gives `[esi+eax]` three times, and cutting
  unrelated earlier code changes the pattern.
- So a diff that is only a swapped SIB pair (same registers, same length) is register/schedule residue. Do not use
  `i[p]`, yoda forms or fake temps for it; only a closer whole-function shape (variables shared or split as in the
  original) can move it. Walls: DrawQuad 0x1014c080, BuildMesh 0x101a82d0, Menu_DrawScene 0x10022830.

## Stack-slot sharing is lifetime based; literal float arguments are temporaries (batch 392)
- Slot groups, not single slots: a dead local's slot is reused by later locals and temporaries with a disjoint
  lifetime (Enemy_StarVolley: dy, the atan2 result temp, the reused turn and motion all sit in one slot in the original).
  Declaration position (function top, block top, inline) of such locals changes nothing (verified: byte-identical
  objects), so "declare later/earlier" is useless for them; only reference counts and lifetimes decide.
- Which group a new local joins: the lowest group whose occupant is dead at the new local's definition. `motion = cond ? 0 : 2`
  stores the 0 before the `fcomp` that reads the compared float, yet cl still shares the compared float's slot
  (AimedShots/StarVolley baseline). Rewriting it `int motion = 0; if (!(x > c)) motion = 2;` changes registers
  (edi as zero) and the frame, so it is not a free spelling.
- Game::MoveCursor (0x10009050) A/B: `float speed; ... speed = 160.0f; ... step.Copy(step * speed);` gives the speed its own slot
  (frame 0x28, 269 diff lines). Writing the literal in each branch, `step.Copy(step * 160.0f);`, makes the float argument a
  compiler temporary: it lands in the slot of the dead `distance`/int-to-float temporaries (frame 0x24 = original), pushes
  and the const store sit in each branch before the jump to the shared tail, as the original. 269 -> 5 diff lines.
  Rule of thumb: an original with `push &tmp` and `mov [tmp], const` inside each branch and a shared call after the
  label is a literal argument per branch, not a named variable. Named block-scope floats compile to the same code.
- /Od (src_od) frame layout is not declaration order either: Names_Expand puts `best` and `matches` low whatever the
  declaration order, and of two scalars (p, found) the later-declared one gets the higher address (-4); the original has
  p lowest although it has fewer references than best, so /Od ordering is not plain reference count.
- The compared `dx`/`dy` pair of an inline atan2 with float parameters comes out mirrored (first-computed in the low slot)
  when written through an `Atan2(float y, float x)` helper versus named locals (first-computed in the high slot, as the
  original AimedShots): named dy then dx matches the original's dy/dx slots; the helper form does not.

## Inline sites, cross-jumped tails, /GS cookies (helper DLLs, batches 399-401)

- **Inline expansion is per call site, by static hotness.** With /O2 /Ob1, cl 13.00 expands an `inline` helper only
  at call sites it rates hot; the rest stay calls. Seven straight-line `GetBit()` calls were all calls, the same seven
  in a loop all expanded. The loop form changes the rating: `for (;;)` rates sites hotter than `while (true)` /
  `while (1)` (Packs DecodeChunk: `while (true)` gives the original's 3 expansions, `for (;;)` gives 4). Caller size
  and definition order (forward declaration, later definition) made no difference. A helper the original never
  expands (PutOneBit) is simply not marked `inline`; marking it changes how its callers schedule around the call.
- **Duplicated tails are cross-jumped.** Writing the same tail in each branch (free + clear + return) lets the
  compiler merge them into one block shared by a later branch (sound3d WaveStream::Close). Writing the tail once
  after an if/else gives a different layout. Which copy survives is not under source control (APP ImeDraw).
- **/GS cookie means a char array of 8 bytes or more.** Arrays of 4 chars or of single-char structs get no cookie.
- **Plain `inline` is not always expanded in big callers;** `__forceinline` is (sound3d MmioFile::Seek). A constructor
  declared `throw()` is not inlined; an in-class definition without it is.
- **Locals live where their scope says.** `for (int y = 0; ...)` instead of a function-scope `int y` changes the
  register choice (APP Screenshot_ReadBackBuffer); declaring a block-local in each block that uses it lets two
  values share one stack slot (Packs MMV3). Dead argument slots are reused last-freed-first (Packs DequantizeBand).
- **A loop the call count proves.** A two-pass `for` stays a loop (one call site) where unrolled calls give two
  (APP Map_Unload: 24 calls vs 25); BMPPack's original even keeps a one-pass loop (`inc; cmp 1; jl`).
- **Macros, not `const float`, for literal operands.** A `const float` object makes cl load the constant first and
  negate at run time; a macro literal gives `fld var; fmul [literal]` and a literal negative (sound3d Update3DPan).
- **Temp registers follow live ranges, so store order matters.** Values stored twice get their register from how long
  they stay live; interleaving the stores as the original did fixed an eax/ecx/edx rotation (APP Render_FillRect).
- **Use the CRT's own inlines.** VC7 math.h `sinf` (an inline over `sin`) gives the original's double store/reload;
  a hand-written `(float)sin(x)` wrapper does not (APP Region_WaveVertex).
- **/Od stack slots follow local names.** In /Od code (sound3d src_od) the slot order comes from the locals'
  names (hash order), not only declaration order; colliding names keep declaration order (first declared = higher
  address). Unused declared locals still take 4 bytes.
- **Write the tail in every branch** (again): Switch_Hold, Text_Substitute, Mp3Stream::Read and OggStream::Read all
  matched once each branch ended with its own copy of the shared statements; an inverted test (`== 0`) also moves
  which arm the compiler places out of line.
- **SIB order across whole loops (D3 study, Menu_DrawScene 0x10022830 / DrawQuad 0x1014c080).** In the originals the operand loaded (defined) last is written first in `[a+b]`: DrawQuad sites `[edx+ebp]`, `[eax+ecx]`, `[ecx+ebx]`, `[edx+ebx]`, `[ecx+eax]` and the three haze loops `[esi+eax]` (esi zeroed after the pointer load) all follow it; ours follows it too except at speculars[i].y and in haze loops 1-2. A/B on the haze loops: dropping any one of the three identical loops gives `[eax+esi]` for the rest, a fourth copy gives `[esi+eax]` for all four, any function-level `int region;` shared by the loops (or declared at the top) gives `[eax+esi]` for all, removing unrelated top-level chunks flips single loops; unreferenced extern declarations and one referenced global before the function change nothing. A standalone loop copy (1-5 loops, struct strides 0x40/0x44, method call with 9 arguments, zero stores, shared or own counters) is always `[esi+eax]`. So the order is register-allocation state of the whole function; there is no spelling at the site.
- **A named local that was a compiler temporary swaps spill slots; so does a function-scope address-taken local** (batch D2, 2026-11-27). (1) Node::ApplyCommandText 0x101a3000: `float degrees = (float)atof(word); limit = degrees * PI / 180` in the six iklim blocks claimed a spill-slot group first and swapped the two slots of every other spilled value (frames / byte-offset induction variable / setanitb mode / the two loop counters: 15 bytes off). Writing `limit = (float)atof(word) * PI / 180` (same `fstp/fld` pair through the cast temporary) matched. A/B: deleting the iklim blocks, or keeping one with the named local, shows the same flip, so a wall of consistently swapped slots may be a named local the original had as an unnamed temporary (cast or call result). (2) Node::ScanChunks 0x101a4a60: `int plugSize;` declared at function top and read through `file->Read(&plugSize, ...)` made cl store `node->plugs[g] = alloc(plugSize, ...)` right after the call (`add esp,8`, store, reload plugSize); declaring `int plugSize;` (and plugCount) in the block that reads it gives the original order (reload, `add esp,8`, pushes, store just before the Read call). Each of the two sites flips alone (30 -> 15 -> 0 diff lines); a small toy did not reproduce it, so treat it as empirical: declare address-taken scratch ints where they are read.

## Turn angle is its own local, not a reused dx/dy (enemy_barrage.cpp, D1 batch)
- Enemy_StarVolley matched once `dy = target - heading; dy = fabs(dy); motion = dy > 1 degree ? 0 : 2` became
  `float turn = target - heading; motion = (float)fabs(turn) > 1 degree ? 0 : 2`. Reusing dx or dy for the turn keeps
  that variable live across the atan2 result, so the compiler temporary lands in the other coordinate's slot and
  motion shares the wrong group. With a separate `turn` the groups come out as the original's: {x-diff, atan2
  temporary, turn} and {y-diff, motion} (AimedShots now has these groups right too; only its two spill slots stay
  swapped).
- The inline `(float)atan2((double)dx, (double)dy)` pattern in the stalk/hover siblings (dash slash, hover call
  strike) stores the y-diff first, then the x-diff, passes both as doubles and puts the result in a float temporary
  that shares the x-diff's slot. A named variable that receives the result (`delta = (float)atan2(..)`) ties the
  result to that variable's slot instead.
- Slot ordering is not plain reference counts: three variables with four references each (int restart, float speed,
  float delta) come out in an order no declaration order changes, and deleting one reference elsewhere can flip two
  spill slots (AimedShots). Treat equal-count slot order as unexplained.

## Early-return clamps and inlined temporaries (Packs DecodeBand, round H11)
- DecodeBand matched with two edits. `dy = blockY + g_blockScanY[i]` was written out at both of its uses instead
  of being a named local; the named local was what swapped the i and blockY slots. The inlined
  ProbabilityFromScale clamp also became `if (p > MAX) return MAX; return p;` instead of `if (p > MAX) p = MAX;`.
  With the assignment form the compiler reloads the spilled scan index after the clamp; with the early return it
  reloads right after the division, as the original does. A ternary in either direction behaves like the
  assignment. Both edits were needed (each alone left 23-24 diffs). The same early-return change did nothing for
  ToRgbBytes' clamp helpers.
- QuantiseBand reuses its dead `scale` parameter for the divisor (`scale *= QUANT_DIVISOR_PER_SCALE`), which
  puts the spill in the argument slot as in the original. That is a partial gain only.
- Round 11's sweep of single-use named cast/call locals over all APP DIFF functions found no further exact
  matches. The rule closes a slot wall only when the named local is the sole cause.

## Single-use ftol values sink past calls (minigame HUDs, round 12)
- cl 13.00 moves a value that is used once (named local, accumulator, inline helper result or plain expression)
  to just before its use, past any call in between. The minigame HUDs compute HalfWidth before the pop draw and keep it
  in ebx, which happens only when the value has a second use: the original writes `RIGHT - halfWidth - width` in both
  the draw and its glow copy. That took DrawHud and DrawHudTwoScores from about 80 diff lines to 8. What is left is
  the eax/ecx choice for the first _itoa call.
- `__forceinline` was needed for CopyString in sound3d _IPlay: plain `inline` expanded it at three queue writers
  but left _IPlay's copy as a call.
- sound3d's inventory had _IPlay (0x10009ad0) merged into _IPlayFadeB, so it is listed in module.json extra_functions.

## Scratch registers: rotation for temporaries, eax first for cross-block values (minigame HUDs, lane C2)
- cl 13.00 hands out eax/ecx/edx to a block's temporaries in turn: each one gets the register after the one allocated
  last, and the turn carries on from the code before (a forced `this` in ecx does not count). Call arguments are
  allocated right to left, so `_itoa(global, buffer, 10)` gets the `lea` of the buffer first and the global's load
  second: buffer in register r, value in r+1, the next temporary in r+2. Toy tests over 120 different lead-ins never
  gave anything else.
- A value that is live across a block boundary is allocated before the block's temporaries and takes eax when it is
  free. The temporaries then skip eax. The minigame HUDs' first `_itoa` has the value in eax and the buffer in the
  next register in turn (eax/ecx/edx in both DrawHud and DrawTimerHud, even though their lead-ins differ). So in the
  original that score is a cross-block value. Proof: `int score = DAT_10c65060; for (int once = 0; once < 1; once++)
  _itoa(score, ...)` makes DrawHud, DrawHudTwoScores and DrawTimerHud byte-identical. DrawHudMinuteTimer needs its
  second score done the same way, which leaves 8 stack-slot lines. That loop is a fakematch and is not used.
- These also create the boundary, but none of them reads as the original: a ternary with identical arms,
  `if (s >= 0 || s < 0)`, an unsigned `if (score >= 0)` around the whole score block, and loops that run once
  (`for`, `while (n--)`). These do not create it: a named local, a local defined further up, `register`/`const`, an
  inline helper (cl substitutes plain arguments), `if (1)`, `do {} while (0)`, `for (;;) {... break;}`, a goto to
  the next line, a struct member, a pointer or a reference to the global.
- The banner half sizes (`halfW, halfH, glowHalfW, glowHalfH`, plus `fade` in the timer HUDs) are declared once at
  the top of the function, C style. As per-branch locals, glowHalfH's spill shared the `step`/`width` slot instead of
  the cast temporaries' slot.

## Two tests of one flag, effect aim callback (lane E2)
- ListBank::LoadText 0x100fb6d0 matched once the line-end test and the blanking were two separate ifs on `inComment`
  (`if (inComment == 1) { if (c == '\r' || c == '\n') inComment = 0; }` then `if (inComment == 1) { blank }`). cl 13.00
  always lays out `if (a || b) A else B` with B first and A (the short block) at the end, whatever the spelling (toys:
  plain, `&&` inverted, switch, do/while(0)+break, continue, return); the original's `je A; jne B; A: ...; jmp join; B:`
  is the layout of `if (a || b) A;` with B as the next statement. The second `cmp edi, 1` disappears because the compiler
  threads the known value of inComment (the same threading makes the comment start jump past the first test).
- The host aim callback 0x10010040 (EffectOptions typed: m_0014 = locked slot, m_0030 = Vec4 out, m_000e = list) and the
  map root's draw callback 0x1000c620 (slots 0x5c..0x7b near the shadow position) matched at the first attempt. A
  `for (slot = 0x3a; slot < 0x7c; ...)` over DAT_1021f3c0[slot] is compiled with a rebased induction variable
  ([esi + 0x1021f3fa], compare on esi + 0x3a): the original really indexes by slot.
- MotionList::InsertByName stays at 41 bytes: the inlined Insert's fail block survives as the *last* copy in the original
  (`jl FAIL; cmp 0x7f; jle OK; FAIL: ...`), the standalone Insert 0x101ad550 only matches with the two separate ifs.

## Stacked case labels vs one body per label (round 15, enemy_voice / char_kind_select)
- When several case labels share one body (`case 3: case 7: ...`), cl 13.00 builds a byte-indexed table and a
  median-pivot tree. Where the original has direct jump tables and cross-jumped tails, it wrote one body per
  label, with the bodies in the original's address order. cl then merges the identical tails itself. This took
  enemy_voice from 32% to 46% and char_kind_select from 44% to 55% fuzzy.
