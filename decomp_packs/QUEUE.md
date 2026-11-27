# Packs.dll queue

Lanes are disjoint by file. Sizes are real extents (bytes). "fp" = x87 instruction count.

## Status after batch 395 (24 of 51 matched)

Matched: PackFile ctor/dtor/Tell/Seek/Open/Read/Write, BMPLoad, BMPSave, ExecUnpack, PutZeroBit, PutOneBit, EncodeStart,
PackBlocks, PackLevels, EmitBit, EncodeBit, ConvertBgrToPlanes, EncodeBandPlanes, DecodeSignedValue; batch 395:
DecodeJpegRgb, GetBit (BitReader struct), AdvanceOverToken, StoreCoefficient (unsigned short run/level stream).

Open, closest first (diffs from build output):
- 10003600 DequantizeBand (10: two stack slots; original reuses dead argument slots)
- 10006350 GetBits (17: one extra movzx)
- 100037e0 ReadStreamHeader (31: edx/eax swap), 10005a40 EncodeJpegRgb (40), 10006350 GetBits (69), 100068d0 PutBits (74)
- 10006950 AdvanceOverToken (98), 100066d0 UnpackRecords (104), 100060a0 ExtractBaseName (138: zero register),
  10005470 SaveJpeg (130), 10005650 MMV3 (208), 10003490 ToRgbBytes (231)
- larger: 100015c0 WriteStream, 10002bc0 PackPlanesToWords, 100019d0 QuantiseBand, 10003bb0 QuantiseBands,
  10004e50 EncodeStream, 10004540 EncodeBand, 10004fb0 BMPPack (level search unrolled 8x), 10003e60 DecodeBand,
  10004120 DequantizeLevels, 10004830 DecodeLevels, 10002d20 InverseWaveletLevel (shape: two written-out passes?),
  10004a70 BMPUnpack (records are now a real byte array; frame layout differs), 10006e90 ExecPack,
  10006a40 EncodeChunk (inline budget), 100063c0 DecodeChunk (shared length tail)
- 10001000 BandStatistics (1488 vs 1470 bytes: two double slots and pointer/index registers swapped)
- 10002180 TransformLevel: behaviour verified bit-exact against the DLL (23-tap filter, 300 random planes), but
  written as a loop; the original spells the 23 taps out as shifting locals (690 instructions). Needs a rewrite.

Notes: encoder globals (0x1003afe0..) and decoder globals (0x1003b008..) are separate variables; the scan tables
`g_scanColumn/g_scanRow` (0x10038040/440, packer) and `g_blockScanX/Y` (0x10038848/c48, unpacker) are separate too.
Globals are extern only (relocations are masked). Helpers need external linkage for compare_obj to see them.
