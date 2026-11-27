# Packs.dll queue

Lanes are disjoint by file. Sizes are real extents (bytes). "fp" = x87 instruction count.

## Lane A: files (src/pack_file.cpp, src/bmp_file.cpp)
- [x] 10006180 9, 10006190 26, 10006210 12, 10006220 24: PackFile ctor, dtor, Tell, Seek
- [ ] 100061b0 85 (open), 100060a0 213 (path helper, /GS), 10006240 121 (read), 100062c0 38 (write)
- [ ] 10005c00 588 BMPLoad, 10005e50 580 BMPSave (exports)
- [ ] 10005900 305, 10005a40 439: JPEG decode/encode through libjpeg (calls into 0x10011ae0..0x10014146)
- [ ] 10005470 465 SaveJpeg (export)

## Lane B: ExecPack compressor (src/exec_pack.cpp)
- [ ] 10006e90 585 ExecPack, 10006e30 81, 10006a40 996, 10006950 235, 100068d0 125, 10006870 89, 10006820 80
- [ ] 100070e0 129 ExecUnpack, 100066d0 321, 100063c0 771, 10006350 109, 100062f0 85

## Lane C: BMPPack (src/bmp_pack.cpp)
- [ ] 10004fb0 1209 BMPPack
- [ ] 100015c0 907, 10002180 2611, 10002bc0 340, 10003d90 206
- [ ] 10003bb0 471 -> 100019d0 1954 (fp 129)
- [ ] 10004490 168 -> 10003ae0 201 -> 10001950 124
- [ ] 10004e50 345 -> 10004db0 147 -> 10004540 737 -> 100043b0 222, 10003a40 159, 10001000 1470 (fp 200)

## Lane D: unpacking (src/bmp_unpack.cpp)
- [ ] 10004a70 823 BMPUnpack, 10005650 682 MMV3 (exports)
- [ ] no direct callers (called through a table): 10002d20 1338, 10003490 359, 100037e0 594, 10004120 656 -> 10003600 474,
      10004830 576 -> 10003e60 689 -> 10003260 558
