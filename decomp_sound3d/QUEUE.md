# sound3d.dll queue

100 of 117 game functions matched (batch 403). Open functions, closest first (fuzzy % from the build):

- 10003640 ?Open@MmioFile@@QAEHPBD0@Z, 1085 bytes, 98%
- 10006810 ?PlaySE@SoundChannel@@QAEHPBDHPAM11111HM@Z, 4140 bytes, 92%
- 10007840 ?Update3DPan@SoundChannel@@QAEHXZ, 774 bytes, 84%
- 10001630 ?LoadSource@Mp3Stream@@QAEXXZ, 222 bytes, 79%
- 10008790 ?InitTrackedAllocator@@YAXXZ, 254 bytes, 65%
- 10008890 _MixOpen, 1663 bytes, 54%
- 10002e70 ?CheckKey@WaveStream@@QAEHPAEHI@Z, 553 bytes, 50%
- 10001ec0 ?RewindDecoder@OggStream@@QAEHXZ, 421 bytes, 48%
- 10006250 ?FindOrLoadSound@@YAHPBD@Z, 1193 bytes, 41%
- 10001710 ?Read@Mp3Stream@@QAEIPAEI@Z, 1226 bytes, 38%
- 10001ce0 ?StartDecoder@OggStream@@QAEHXZ, 470 bytes, 21%
- 10001000 ?GetBaseFileName@@YAXPADPBD@Z, 240 bytes, 20%
- 10004c20 ?Read@WaveStream@@QAEHPAEH@Z, 2154 bytes, 13%
- 10007dd0 ?Open@SoundChannel@@QAEHPBDHHHI@Z, 693 bytes, 12%
- 10001500 ?BuildWavHeader@Mp3Stream@@QAEXPBUWavSourceFormat@@@Z, 270 bytes, 12%
- 10008360 ?Init@SoundChannel@@QAEHPAUHWND__@@PAUIDirectSound8@@PAUIDirectSoundBuffer@@@Z, 673 bytes, 11%

Notes:
- Batch 399: SetListener (ternary fallbacks), ApplyVolume (separate early returns), WaveStream::Close (two identical
  decoder tails the compiler cross-jumps), SetPosition (10007c50; its mangled name was wrong in obj_symbols.tsv).
- IsPlaying: 11 bytes, only a dead `mov ecx,[esi+0x204]` after GetTickCount is missing.
- Update3DPan: instruction-identical; pan lands in frontAngle's dead slot in the original.
- Batch 400: GetPosition (members reread at each use, no caching locals), WaveStream::Tell, WaveStream::Open
  (MmioFile::Seek __forceinline, OggStream ctor in-class without throw()), CheckNameKeyA, MixerControls ctor 10008830
  (added via module.json extra_functions; the function list is now 117).
- InitTrackedAllocator returns int; the original has a /GS cookie with no visible array.
- ProcessCommandQueue (111 bytes): plain g_commandQueue[other] indexing gives the original's .command pointer walk;
  the IPlaySEB tail still differs (original shares `lea eax,name; push; call` across both paths).
- SoundChannel::Open (103 bytes): separate early returns give the shrink-wrapped saves; format field order left.
- Batch 402: WaveStream::Read now 2128 vs 2154 bytes (DecodeNextBlock __forceinline: the third site was missing; only
  four stack slots differ); Update3DPan 85 bytes (windows.h min/max, constants as macros); SoundChannel::Open 52 bytes.
- Batch 403: IsPlaying (dead timestamp check, both outcomes return 0), ProcessCommandQueue (queue entry read in
  place, one IPlaySEB call per branch), OggStream::Read (each branch finishes its own state updates: cross-jumped).
- InitTrackedAllocator: a char[8] local reproduces the cookie exactly, but no real use for one is known yet.
- MmioFile::Open: original has plainPath at 0x140 and datPath at 0x340 (ours reversed); declaration order has no effect.
- OggStream::Read: `lastBlock - skipped + position` compiles as (position - skipped) + lastBlock in ours.
- 
- MmioFile::Open: 10 bytes, only the stack order of datPath/plainPath (all three path buffers have 3 references).
- SoundChannel::PlaySE (src_od/, built /Od /Op-): structurally identical, frame 0x110 vs 0x114 (the /Od slot order with /GS).
- Mp3Stream::Read: the original shares the two "finish a pending block" tails and keeps the early return out of line,
  which suggests a common tail or inline helper in the source.
- WaveStream::Open tests "2"/"3" twice: kept (the binary pairs every suffix as lower/upper case; digits repeat).

## Round S8 notes
- Mp3Stream::Read MATCH. Levers: test `outputUsed == 0` (then-arm inline, else-arm out of line); each branch ends with its own `position += step; lastPosition = position;` (tails cross-jump); statement order inside arms changes store scheduling.
- /Od (src_od) stack slot order depends on local NAMES (hash-table order), not only declaration order; colliding names follow declaration order (first declared = higher address). Unused locals still reserve 4 bytes.
- PlaySE: original has one extra unreferenced 4-byte slot (frame 0x114 vs ours 0x110). A dummy `int spare;` fixes the frame (score 303 -> 121) but is a fake local, not applied.
- Walls: SoundChannel::Open (scheduler), Update3DPan (/Op ternary temp), LoadSource, BuildWavHeader, GetBaseFileName (register assignment), MmioFile::Open (slot order), WaveStream::Read (slot sharing; `file.Read(buf, bytes, 1)` push order improves 146->144), RewindDecoder (esi/edi swap), Init (device spill).

Round H9 (duplicated tails / in-place reads / inverted tests / statement order over the open functions):
- WaveStream::CheckKey: 127 -> 42 differing bytes. The 31 key assignments are order-independent; a move sweep (single
  moves, then random walk) found that swapping key[25]/key[16] and moving key[24] behind key[27] gets the whole first half
  and most of the second byte-identical. Remaining: the CSE temps of name[6]/name[3] (slots 0x17/0x18 and two reloads of
  name[6] in the original) and the final eax/ecx of the checksum compare. Applied (two lines of file_name_utils.cpp).
- BuildWavHeader: instruction-level metric 16 -> 11 with a reordered header store sequence (riffTag bytes, fmtTag,
  waveTag shuffled); not applied: the order is unreadable and still not a match. The original keeps `(WORD)channels`,
  `(WORD)bitsPerSample` in dx/ax and multiplies by sampleRate before the first store; WORD/int locals for them made it
  worse (189-227 diffs).
- LoadSource: all 24 orders of the four format stores and in-place `info.samplesPerFrame` tried; best 36 diffs (bits first,
  then sampleRate/channels), original 47 -> no match; the second hunk (argument registers of mp3_decoder_start) follows.
- MmioFile::Open: datPath/arcPath/lookup declaration order, plainPath at function top: no effect (still 10 bytes, only
  the datPath/plainPath slot swap).
- GetBaseFileName (and Packs ExtractBaseName, same helper): the original keeps baseName in ebx and nameStart/dot/i in
  ebp/edi/esi, zero-stores through the zero register; all 24 orders of the four initialisations and pointer-difference
  spellings of nameStart give the same bytes (ours already emits the pointer-difference form).
- Update3DPan: min/max argument orders (8 variants) are the same bytes; the original keeps the first max() result in a
  float temp (slot 0x1c) before the min compare, ours folds it.
- RewindDecoder/StartDecoder: `for` header, dropped `continue`, pointer locals os/op/vc removed: no change. The original
  tests `headerPackets < 2` before the first iteration (no constant fold) and spills `this` to the stack.
- InitTrackedAllocator: only the unexplained /GS cookie (4 byte slot + xor) separates it; the inlined strcpy("Unknown")
  already matches.

Round H11 (named locals that were temporaries, function-scope address-taken locals, reused variables), no new match:
- MmioFile::Open: moving every non-buffer local, the address-taken tables, `wanted`, `directory` or all of them into the
  block that uses them leaves the same 10 bytes (original low to high: directory, plainPath, arcPath, datPath; ours
  directory, datPath, arcPath, plainPath; declaration order and scope make no difference to the buffer order).
- Update3DPan: `lowClamped` named local, in-place pan, unnamed clamp or volume, min/max swapped: same bytes. The
  original loads the pan into ecx before the first compare and keeps the max() result in a copy at [esp+0x1c].
- FindOrLoadSound: `window` moved into the archived branch (three positions) and one index per loop: same code (the
  original frame is 4 bytes larger; its `slot` lives at [esp+0x14] and the scan pointer is spilled into [esp+0x10], the
  `window` bytes).
- SoundChannel::Open: `stereo`/`startBytes` unnamed or `created` in its block: all worse (the field order found by
  search stays).
- LoadSource/BuildWavHeader/GetBaseFileName/CheckKey/InitTrackedAllocator: no named cast/call temporaries to remove; unchanged.

## Round H12 (slot-order and inline-site levers over the open list)
- _IPlay (0x10009ad0) MATCH. Two levers: (1) `CopyString` is `__forceinline` (a plain `inline` expanded it at the three
  other queue writers but left IPlay's copy a call; the original has the inline `mov cl,[edx]; mov [esi+edx],cl; inc; test;
  jne` loop there too). (2) The integrity compare reads `storedSum != sum || storedMask != mask` (stored value first);
  `sum != storedSum || mask != storedMask` gave the same instructions but swapped the slots of the four address-taken
  words (original ascending: mask, storedSum, storedMask, sum). Declaration order and local names had no effect on
  these /O2 slots; only the operand order of the compare did. The function is not in the 117-function triage list, so
  the module report count stays 101 of 117; the build prints MATCH for it.
- PlaySE (src_od/, /Od): the frame layout is now understood. /Od assigns frame slots by a hash of the local NAME
  (colliding names: first declared = higher address), and puts the /GS cookie just above the first struct in that
  order. A harness (one function with many referenced locals, `cl /Od /GS /FAs`, read the `_name$ = -N` equates)
  shows the order is stable whatever else is declared, so candidate names can be ranked once and chosen. The original's
  function-scope order (high to low) is created, mono, dataBytes, slot, flags, cookie, description, stereo, result,
  unusedFlag, releaseResult; the names dsBuffer, isMono, bufferBytes, index, options, bufferConfig, stereo, ret,
  loopFlag, refs reproduce it (all offsets down to -0x4c now equal the original's). Block locals still differ: the
  original orders block A (soft pan) as region1, region2, size2, lockResult, size1, relativeX, attenuation, frontAngle,
  pi, distance, pan, soundAngle, relativeY and block B as region1, region2, size2, lockResult, size1 (their names
  are not searched yet). PI_FLOAT was one ulp off (0x40490fdb; the original stores 0x40490fda = 3.1415925f), fixed.
  The frame is still 0x110 against 0x114: the original has one 4-byte slot at -0x98, between block B's scalars and the
  DS3DBUFFER, that no instruction references (an unreferenced local in that block). Not added: an unreferenced local is
  a dummy. 324 -> 245 differing bytes.
- Tried without effect: RewindDecoder/StartDecoder with named `buffer` (the esi load moves after the call as in the
  original, but the `this` spill disappears), os/op/vc pointer locals (same bytes), `headerPackets` initialised at the
  top (worse). In the original `this` is spilled and reloaded at every use because vc (ebx), os (ebp), op (edi) and the
  counter (esi) all stay in registers; ours rematerialises `this+const` addresses instead of holding os/vc. No spelling
  found that makes cl hold them.
- SoundChannel::Init: `device` assigned in each branch (`device = &directSound` after the test, as the original puts the
  `lea` inside both arms) matches the first 40 bytes, but cl then folds the shared store and rematerialises `device`,
  where the original keeps it in a stack slot (frame 0x80). Not applied (428 against 434 diffs only).
- _MixOpen: QueryControlId with an out parameter (`*controlId = ...` in each branch, which would give the original's
  store-per-branch) was bigger (1792 against 1696 bytes, original 1663); the original also keeps 0x94 and the control
  address in registers across the sites. Not applied.

## Round H13

- Inventory: 23 missed starts and 15 built-but-unlisted functions added through module.json extra_functions; triage rerun (156 functions).
- New MATCH: 6 ATL thunks, AtlA2WHelper/AtlW2AHelper, Allocate, CTempBuffer/CW2AEX small methods, MmioFile ctor, OwnedText ctor/dtor, Mp3Stream ReadFileBytes/Seek/LoadSource, OggStream::Seek.
- Lever: a referenced in-class inline member gets an out-of-line COMDAT copy even when expanded everywhere, so never-called originals match by referencing it from a matched function. ATL thunks match by taking their address in a const pointer.
- Walls: WaveStream::Read (stack slots, 629 diffs), PlaySEExtended (9 byte diffs, early-return guard layout; forms A/B/C/E/G of the guard tried), MmioFile::Open buffer order, EncodeStream/EncodeChunk/EncodeJpegRgb.
- Not reached: remaining open functions in decomp_packs (no change, 34 of 51).

## Round H14

- New MATCH: WindowHolder::ClearWindow (0x10009400) and WindowHolder::Release (0x10009410), the two uncalled methods (g_window = 0 returning this, and an empty one) in src/sound_window_holder.cpp with include/window_holder.h; 133 -> 135 of 156 (the build counts 135).
- 0x10001210 is MmioFile::Seek (the dead out-of-line copy, mapped in obj_symbols.tsv): all but the register of the final target differ (original keeps the target in edx and does add edx,[esp+4]; ours eax plus a reload). Tried offset+target, target+offset, early CUR test, ternary base: same or worse. 20 byte diffs, the inline expansions keep matching.
- PlaySEExtended: guard forms tried this round on top of A/B/C/E/G: nested ifs with one return 0 at the end, `||` guard, `else if`, ready-first, inline guard helper. The original keeps the FIRST xor/ret block (je L; xor eax,eax; ret; L: ...; je back to it); ours keeps the later copy or puts return 0 last (176 bytes). Not under source control (see NOTES "which tail copy cl keeps"). Unchanged, 9 diffs.
- Not reworked (no new idea): Update3DPan, FindOrLoadSound, SoundChannel::Init, _MixOpen, RewindDecoder/StartDecoder, WaveStream::Read.

## Round H16

- New MATCH (135 -> 138 of 156): MmioFile::Seek out-of-line copy 0x10001210, SoundChannel::Update3DPan 0x10007840, ATL::AtlThrow 0x10001610.
- Seek: each branch adds the offset to its own base (`mmioSeek(handle, baseOffset + offset, SEEK_SET)` for SEEK_SET, `baseOffset + size + offset` for SEEK_END). cl merges the common tail itself (`add edx,[esp+4]` + call), which puts the target in edx. A shared named `target` (assigned per branch, offset added after the join) is a cross-block value and always lands in eax. Same lesson as "write the tail in every branch".
- Update3DPan: the pan clamp is `min(max(-PAN_FULL, pan * SOFT3D_PAN_SCALE), PAN_FULL)` with the product written out inside the macro, no `scaledPan`/`clampedPan` locals. The macros repeat the product; the original's float temp at [esp+0x1c] and its un-folded second evaluation of max() come from that. Named locals (scaled, clamped, in-place pan) gave 85-102 diff bytes; the unnamed expression is byte-identical.
- AtlThrow: the original is MFC's `if (hr == E_OUTOFMEMORY) AfxThrowMemoryException(); else AfxThrowOleException(hr);` (atlexcept.h under `_AFX`). `#include <afx.h>` before atlbase.h in atl_conversion.cpp and atl_string_fakes.cpp reproduces it; ours had thrown a CAtlException.
- GetParam/SetParam (sound_mixer.cpp, not matched, structure closer): master line index test is a `switch` (original `sub eax,5; je; dec eax`), the result is a second local `level` (the original loads the address-taken `value` into esi and negates there). SetParam: the control value buffer is a copy (`controlValue`, address taken; the parameter `value` stays in ebp for the cache store, the cache pointer is a CSE spilled into the dead `index` slot), which made body and registers identical to the original. Left: GetParam register order (original: kind ebp, index esi, slot ebx; ours kind ebx), the original keeps the first `return 0` epilogue (after the mixerOpen success return) where ours keeps the last; SetParam prologue order (`mov eax,ebx; shl eax,6` before the `push esi`) and the epilogue schedule of `*cached = value; return 1`.
- Not closed, new hypotheses tried without effect: PlaySEExtended (`&&` nesting, nested else returns, braces: same 9 bytes or the 176-byte layout), InitTrackedAllocator (a `char label[8]` copied to the global: 55-110 diff bytes, cookie frame not reproduced), MmioFile::Open (all six orders of the three path buffers, with directory before/after them: the buffers sit in first-use order whatever the declaration order; the original has them in reverse first-use order), FindOrLoadSound (the original keeps the zero in ebp for the whole function and `slot` in esi with a stack home; ours gives ebp to `slot` and rematerialises zero), SoundChannel::Open (the 720 orders of the six format stores were searched, best 27 diff bytes (applied: nBlockAlign before wBitsPerSample, was 52), no match; the original loads bitsPerSample into cx right after the nBlockAlign store and moves `mov ecx,9` after it, ours loads it into ax last).

## Round H17 (register-role walls; 138 -> 140 of 156)

- New MATCH: _GetParam 0x10008f30 and _SetParam 0x100090f0 (src/sound_mixer.cpp). Levers, in the order they mattered:
  (1) No `slot` local in GetParam: `kind * MIXER_PARAM_STRIDE + index` is written out at each use (macro PARAM_SLOT; cl
  CSEs it into one web). The named local gave the slot a higher priority than the kind and the kind took ebx; the
  original has index esi, slot ebx, kind ebp. A probe (extra `g_paramValue[slot]` references at the top) shows the gap was
  under one entry-weight reference: one extra reference made the slot outrank the index, none left it behind the kind.
  (2) The control id is never returned from inside the switch: unknown (kind, index) leave `details.dwControlID` at
  MIXER_NO_CONTROL (switch arms are `if (index == VOLUME) id = ...;` / inner switch without `default`), one
  `if (details.dwControlID == MIXER_NO_CONTROL) return 0;` after it. With `return 0` in every default cl kept the LAST
  copy of the shared return-0 epilogue (at the end); with a single check the original's layout comes out (the return-0
  epilogue right after the mixerOpen success block, the handle == 0 path joining the final epilogue at eax = 0).
  SetParam: (3) the guard is two statements, `if (g_paramValue[...] == value) return 0; if (handle == 0) return 0;`; the `||`
  form put `shl eax,6` after `mov esi` and spilled the cache pointer late (271 -> 13 diff bytes); (4) the value copy
  `controlValue = value` comes after `details.cChannels = 1` (its store sits right before the call in the original).
  Rule of thumb confirmed: before touching declaration order, drop the named local that only names a CSE'd expression, and
  turn `return 0` defaults into fall-through to one shared check.
- QuantiseBand (Packs): the roundOffset/reconstructionOffset swap is a near tie. One extra reference to
  reconstructionOffset at the top (any `if (reconstructionOffset == k)`) flips them to the original's roles (round ebp, recon
  ebx), so the gap is below one entry-weight reference; but the unnamed-expression forms `((scale - 1) / 2)` /
  `((quality - 1) / 2)` (hoisting changes, 1665 diffs), a nested-vs-early-return Measure helper, `level == 0` early return,
  and declaring reconstructionOffset first (roles unchanged, only the computation order moves) do not make the flip. The
  pass-two coefficient web also takes ebx where the original gives it ebp (zero constant and the coefficient share ebp
  there). Slot order of the six address-taken sums/counts does not follow declaration order (sums-first and counts-first
  both give counts at 0x10) and is not a function of the local names at /O2.
- _MixOpen: the original keeps &g_mixerControl in ebp (2 uses per site: memset and pamxctrl) and 0x94 in ebx, the import
  pointer in esi, and the 0x18 cbStruct as an immediate; ours keeps 0x18 in ebp and the control address as an immediate
  (ebx = 0x94, esi = import as the original). So an address constant is promoted in the original and not in ours although
  both have two uses per site; not reproduced (no spelling of g_mixerControl/QueryControlId tried so far changes it).
- RewindDecoder: ours has op = esi and the counter = edi, the original op = edi and counter = esi (vc ebx, os ebp equal). The
  counter needs more weight than `op`; `++headerPackets`, `while (++headerPackets < 2)`, unsigned counter, os/op/vc locals
  and declaration order change nothing, an extra reference to the counter after the loops changes the picture (this no
  longer reloaded into edi) without reaching the original. The original also keeps the top-of-loop test
  (`cmp esi,2; jge; jmp body`) that cl folds in ours.
- PlaySEExtended: the original keeps the FIRST `xor eax,eax; ret` (je over it) and the ready test jumps back to it; ours
  keeps the second. Early returns, `else if`, `== 0`, ready-first nesting and a single trailing `return 0` (176 bytes) tried
  again; the sibling IPlaySEB (return -1) compiles like ours, so the zero return in the original is not the same IL.
- GetBaseFileName: original holds baseName in ebx, i esi, nameStart ebp, dotPos edi and stores the memset zeros through
  esi (i's zero); ours uses scratch registers for dotPos and a separate zero. Strlen result into a `length` local,
  conditional expression inside the index and memset-before-initialisers did not move it (135-150 diffs against 139).
- SoundChannel::Open: the 27 diff bytes are scratch-register choices (eax vs ecx for the DSBCAPS flag lea, ecx vs edx
  for the CreateSoundBuffer vtable load and the stereo*3 lea) plus the `push 0` placement; a named `flags` local, the
  ternary flag forms, `!= 2` arm order and `+` operand order leave them unchanged (moving lpwfxFormat is far worse).
- Not reworked (no new idea within the lane): FindOrLoadSound (its archived branch differs structurally, not only in the
  zero constant's register), StartDecoder.
