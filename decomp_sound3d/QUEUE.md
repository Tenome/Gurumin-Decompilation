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
