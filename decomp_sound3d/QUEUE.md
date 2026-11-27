# sound3d.dll queue

84 of 116 game functions matched (batch 397). Open functions, closest first (fuzzy % from the build):

- 10003640 ?Open@MmioFile@@QAEHPBD0@Z, 1085 bytes, 98%
- 10004140 ?PrepareBuffers@WaveStream@@QAEHH@Z, 590 bytes, 98%
- 10004390 ?OpenWav@WaveStream@@QAEHPBD@Z, 507 bytes, 98%
- 10003ab0 ?Open@Mp3Stream@@QAEHPBD@Z, 497 bytes, 96%
- 10007840 ?Update3DPan@SoundChannel@@QAEHXZ, 774 bytes, 84%
- 1000a3a0 ?ProcessCommandQueue@@YAXH@Z, 843 bytes, 83%
- 100030a0 ?Close@WaveStream@@QAEHXZ, 1203 bytes, 81%
- 10003cb0 ?Open@OggStream@@QAEHPBD@Z, 452 bytes, 79%
- 10001630 ?LoadSource@Mp3Stream@@QAEXXZ, 222 bytes, 79%
- 100058f0 ?IsPlaying@SoundChannel@@QAEHXZ, 116 bytes, 71%
- 10002c50 ?Tell@WaveStream@@QAEHXZ, 222 bytes, 70%
- 10004860 ?Open@WaveStream@@QAEHPBD@Z, 950 bytes, 70%
- 10008790 ?InitTrackedAllocator@@YAXXZ, 254 bytes, 65%
- 10008890 _MixOpen, 1663 bytes, 54%
- 10002e70 ?CheckKey@WaveStream@@QAEHPAEHI@Z, 553 bytes, 50%
- 10001610 ?AtlThrow@ATL@@YGXJ@Z, 28 bytes, 50%
- 10001ec0 ?RewindDecoder@OggStream@@QAEHXZ, 421 bytes, 48%
- 10003e80 ?Read@OggStream@@QAEHPAEH@Z, 592 bytes, 47%
- 10005990 ?SetListener@SoundChannel@@QAEHMMMMMMMMMM@Z, 305 bytes, 44%
- 10002d60 ?CheckNameKeyA@@YAHPADH0HI@Z, 264 bytes, 27%
- 10001ce0 ?StartDecoder@OggStream@@QAEHXZ, 470 bytes, 21%
- 10001000 ?GetBaseFileName@@YAXPADPBD@Z, 240 bytes, 20%
- 10005cd0 ?ApplyVolume@SoundChannel@@QAEHHH@Z, 170 bytes, 17%
- 10004c20 ?Read@WaveStream@@QAEHPAEH@Z, 2154 bytes, 13%
- 10007dd0 ?Open@SoundChannel@@QAEHPBDHHHI@Z, 693 bytes, 12%
- 10001500 ?BuildWavHeader@Mp3Stream@@QAEXPBUWavSourceFormat@@@Z, 270 bytes, 12%
- 10008360 ?Init@SoundChannel@@QAEHPAUHWND__@@PAUIDirectSound8@@PAUIDirectSoundBuffer@@@Z, 673 bytes, 11%
- 10005b40 ?GetPosition@SoundChannel@@QAEHXZ, 279 bytes, 9%
- 10001710 FUN_10001710, 1226 bytes, 0%
- 10006250 FUN_10006250, 1193 bytes, 0%
- 10006810 FUN_10006810, 4140 bytes, 0%
- 10007c50 FUN_10007c50, 382 bytes, 0%

Notes: WaveStream::Open tests the "2" and "3" suffixes twice each because the original does; check that this
is the real source shape before it can count. Lane 2 wrote DecodeNextBlock as an inline member that Read expands three
times; verify. 10006810 (4140 bytes, x87, calls sin) and 10006250 (sound cache load) are the largest open functions.
