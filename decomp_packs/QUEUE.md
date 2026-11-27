# Packs.dll queue

Lanes are disjoint by file. Sizes are real extents (bytes). "fp" = x87 instruction count.

## Status after batch 399 (27 of 51 matched)

Matched: PackFile ctor/dtor/Tell/Seek/Open/Read/Write, BMPLoad, BMPSave, ExecUnpack, PutZeroBit, PutOneBit, EncodeStart,
PackBlocks, PackLevels, EmitBit, EncodeBit, ConvertBgrToPlanes, EncodeBandPlanes, DecodeSignedValue, DecodeJpegRgb, GetBit,
AdvanceOverToken, StoreCoefficient; batch 399: GetBits (unsigned short accumulator), PackPlanesToWords (carry bits kept
in unsigned short locals), ReadStreamHeader (declaration position of `consumed`).

Open, closest first (diffs from build output; all register/slot residues unless noted):
- 10003600 DequantizeBand (10: two temp slots swapped; original parks plane1 in the dead levelScale slot; permuter ~1000
  compiles found nothing)
- 10005a40 EncodeJpegRgb (40: ours biases srcRow by +2; rowBuffer/stride swap ebx/ebp), 10005470 SaveJpeg (130, same loop)
- 100068d0 PutBits (74: index/count in edi/ebp, edx=2 hoisted), 100066d0 UnpackRecords (104: ebx/ebp swap)
- 100060a0 ExtractBaseName (138: zero register reused for the memset stores), 10003490 ToRgbBytes (231)
- 10003bb0 QuantiseBands (96 lines: original frame 0x30 vs 0x2c, band offsets spilled to dead argument slots)
- 10004540 EncodeBand (180 lines: consumed/position on the stack, count in edx), 100019d0 QuantiseBand (frame 0x4c vs 0x48)
- 100015c0 WriteStream (register rotation), 10004e50 EncodeStream (119 instr; band[1] is the induction pointer)
- 10004fb0 BMPPack: original has a /GS cookie, so it has a local char array somewhere; locals 8 bytes smaller
- 10001000 BandStatistics (1488 vs 1470: ebx scratch then re-zeroed; previous in edi)
- 10002180 TransformLevel: behaviour bit-exact (loop form). Original: 12-sample preloaded window, 12 row pointers in the
  column pass, hand-factored filter expression, three reflection branches. Needs a full transcription.
- not attempted yet: 10003e60 DecodeBand, 10004120 DequantizeLevels, 10004830 DecodeLevels, 10002d20 InverseWaveletLevel,
  10004a70 BMPUnpack, 10005650 MMV3, 10006e90 ExecPack, 10006a40 EncodeChunk (inline budget), 100063c0 DecodeChunk

Notes: encoder globals (0x1003afe0..) and decoder globals (0x1003b008..) are separate variables; the scan tables
`g_scanColumn/g_scanRow` (0x10038040/440, packer) and `g_blockScanX/Y` (0x10038848/c48, unpacker) are separate too.
Globals are extern only (relocations are masked). Helpers need external linkage for compare_obj to see them.
Declaration position of a later-initialised local changes register assignment; try it before expression rewrites.
A permuter adapter for this tree: compile.cmd/score.py pointed at decomp_packs (see ../tools/gurumin_permuter).
