# Packs.dll queue

Lanes are disjoint by file. Sizes are real extents (bytes). "fp" = x87 instruction count.

## Status after batch 393 (20 of 51 matched)

Matched: PackFile ctor/dtor/Tell/Seek/Open/Read/Write, BMPLoad, BMPSave, ExecUnpack, PutZeroBit, PutOneBit, EncodeStart,
PackBlocks, PackLevels, EmitBit, EncodeBit, ConvertBgrToPlanes, EncodeBandPlanes, DecodeSignedValue.

Open, closest first (diffs from build output):
- 10001950 StoreCoefficient (3 bytes: cl narrows `(0x800 - v) << 4` to a word add)
- 10003600 DequantizeBand (22: two stack slots; original reuses a dead argument slot)
- 10005900 DecodeJpegRgb (15: edi/esi swap), 100062f0 GetBit (23: bit buffer kept in memory)
- 100037e0 ReadStreamHeader (31: edx/eax swap), 10005a40 EncodeJpegRgb (40), 10006350 GetBits (69), 100068d0 PutBits (74)
- 10006950 AdvanceOverToken (98), 100066d0 UnpackRecords (104), 100060a0 ExtractBaseName (138: zero register),
  10005470 SaveJpeg (130), 10005650 MMV3 (208), 10003490 ToRgbBytes (231)
- larger: 100015c0 WriteStream, 10002bc0 PackPlanesToWords, 100019d0 QuantiseBand, 10003bb0 QuantiseBands,
  10004e50 EncodeStream, 10004540 EncodeBand, 10004fb0 BMPPack (level search unrolled 8x), 10003e60 DecodeBand,
  10004120 DequantizeLevels, 10004830 DecodeLevels, 10002d20 InverseWaveletLevel (shape: two written-out passes?),
  10004a70 BMPUnpack (byte buffer + typed view only to get /GS: rework before it can count), 10006e90 ExecPack,
  10006a40 EncodeChunk (inline budget), 100063c0 DecodeChunk (shared length tail)
- not written: 10001000 BandStatistics (least-squares fit; Ghidra dropped ~150 x87 instructions, read the asm),
  10002180 TransformLevel (3x3 wavelet filter, 2611 bytes)

Notes: encoder globals (0x1003afe0..) and decoder globals (0x1003b008..) are separate variables; the scan tables
`g_scanColumn/g_scanRow` (0x10038040/440, packer) and `g_blockScanX/Y` (0x10038848/c48, unpacker) are separate too.
Globals are extern only (relocations are masked). Helpers need external linkage for compare_obj to see them.
