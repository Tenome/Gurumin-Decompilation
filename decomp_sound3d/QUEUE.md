# sound3d.dll queue

97 of 117 game functions matched (batch 400). Open functions, closest first (fuzzy % from the build):

- 10003640 ?Open@MmioFile@@QAEHPBD0@Z, 1085 bytes, 98%
- 10006810 ?PlaySE@SoundChannel@@QAEHPBDHPAM11111HM@Z, 4140 bytes, 92%
- 10007840 ?Update3DPan@SoundChannel@@QAEHXZ, 774 bytes, 84%
- 1000a3a0 ?ProcessCommandQueue@@YAXH@Z, 843 bytes, 83%
- 10001630 ?LoadSource@Mp3Stream@@QAEXXZ, 222 bytes, 79%
- 100058f0 ?IsPlaying@SoundChannel@@QAEHXZ, 116 bytes, 71%
- 10008790 ?InitTrackedAllocator@@YAXXZ, 254 bytes, 65%
- 10008890 _MixOpen, 1663 bytes, 54%
- 10002e70 ?CheckKey@WaveStream@@QAEHPAEHI@Z, 553 bytes, 50%
- 10001ec0 ?RewindDecoder@OggStream@@QAEHXZ, 421 bytes, 48%
- 10003e80 ?Read@OggStream@@QAEHPAEH@Z, 592 bytes, 47%
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
- ProcessCommandQueue: typed now (SoundCommand *later); the original walks a pointer to .command and reads name at -0x100.
- MmioFile::Open: original has plainPath at 0x140 and datPath at 0x340 (ours reversed); declaration order has no effect.
- OggStream::Read: `lastBlock - skipped + position` compiles as (position - skipped) + lastBlock in ours.
- SoundChannel::Open: shrink-wrapped saves (esi at entry, ebx/edi/ebp later). 
- MmioFile::Open: 10 bytes, only the stack order of datPath/plainPath (all three path buffers have 3 references).
- SoundChannel::PlaySE (src_od/, built /Od /Op-): structurally identical, frame 0x110 vs 0x114 (the /Od slot order with /GS).
- Mp3Stream::Read: the original shares the two "finish a pending block" tails and keeps the early return out of line,
  which suggests a common tail or inline helper in the source.
- WaveStream::Open tests "2"/"3" twice: kept (the binary pairs every suffix as lower/upper case; digits repeat).
