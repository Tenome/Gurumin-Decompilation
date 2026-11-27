# Packs.dll queue

Lanes are disjoint by file. Sizes are real extents (bytes). "fp" = x87 instruction count.

## Status after batch 401 (32 of 51 matched)

Matched: PackFile ctor/dtor/Tell/Seek/Open/Read/Write, BMPLoad, BMPSave, ExecUnpack, PutZeroBit, PutOneBit, EncodeStart,
PackBlocks, PackLevels, EmitBit, EncodeBit, ConvertBgrToPlanes, EncodeBandPlanes, DecodeSignedValue, DecodeJpegRgb, GetBit,
AdvanceOverToken, StoreCoefficient; batch 399: GetBits (unsigned short accumulator), PackPlanesToWords (carry bits kept
in unsigned short locals), ReadStreamHeader (declaration position of `consumed`); batch 400: PutBits (the original
inlines PutZeroBit but calls PutOneBit), UnpackRecords (no byte-typed `moreChunks` local); batch 401: MMV3 (block-scoped blockOffset), DequantizeBand
(scaledLevel hoisted), DecodeChunk (`while (true)` outer loop: see NOTES "Inline sites").

Open, closest first (diffs from build output; all register/slot residues unless noted):
- 10005a40 EncodeJpegRgb (40: ours biases srcRow by +2; rowBuffer/stride swap ebx/ebp), 10005470 SaveJpeg (130, same loop)
- 100060a0 ExtractBaseName (138: zero register reused for the memset stores), 10003490 ToRgbBytes (231)
- 10003bb0 QuantiseBands (96 lines: original frame 0x30 vs 0x2c, band offsets spilled to dead argument slots)
- 10004540 EncodeBand (180 lines: consumed/position on the stack, count in edx), 100019d0 QuantiseBand (frame 0x4c vs 0x48)
- 100015c0 WriteStream (register rotation), 10004e50 EncodeStream (119 instr; band[1] is the induction pointer)
- 10004fb0 BMPPack: original has a /GS cookie, so the 292-byte model set holds a char array of >= 8 bytes
- 10001000 BandStatistics (1488 vs 1470: ebx scratch then re-zeroed; previous in edi)
- 10002180 TransformLevel: transcribed in the original's shape (23 named window locals, 12 column pointers), behaviour
  checked against the loop version on 400 random planes; 2576 vs ~2976 bytes, register/slot allocation differs
- 10003e60 DecodeBand (43: scan-loop i in edx vs ecx, blockY slot)
- 10006e90 ExecPack (403: reload block before the loop top, two epilogues), 10006a40 EncodeChunk (510: the original
  also expands PutOneBit once at the match flag; run length in eax)
- 10004a70 BMPUnpack (761: original caches the GetTickCount import in edi, data in ebp)
- 10004120 DequantizeLevels, 10004830 DecodeLevels (original frames larger, hoisted address temps)
- 10002d20 InverseWaveletLevel: two hand-expanded passes with plane[row*stride + i] addressing; needs a transcription

Notes: encoder globals (0x1003afe0..) and decoder globals (0x1003b008..) are separate variables; the scan tables
`g_scanColumn/g_scanRow` (0x10038040/440, packer) and `g_blockScanX/Y` (0x10038848/c48, unpacker) are separate too.
Globals are extern only (relocations are masked). Helpers need external linkage for compare_obj to see them.
Declaration position of a later-initialised local changes register assignment; try it before expression rewrites.
A permuter adapter for this tree: compile.cmd/score.py pointed at decomp_packs (see ../tools/gurumin_permuter).
